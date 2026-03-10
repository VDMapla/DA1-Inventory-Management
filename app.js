// app.js

/**
 * INVENTORY MANAGEMENT SYSTEM - SIMPLIFIED API BRIDGE
 * 
 * Interacts with the C WebAssembly Backend via simple string/integer wrappers.
 */

document.addEventListener("DOMContentLoaded", () => {
    
    // --- UI Elements ---
    const navItems = document.querySelectorAll('.nav-item');
    const views = document.querySelectorAll('.view-section');
    const notification = document.getElementById('notification');
    const overlay = document.getElementById('loading-overlay');
    const mainApp = document.getElementById('main-app');
    const consoleTerminal = document.getElementById('system-console');

    // Navigation logic
    navItems.forEach(item => {
        item.addEventListener('click', (e) => {
            e.preventDefault();
            navItems.forEach(n => n.classList.remove('active'));
            views.forEach(v => v.classList.remove('active'));
            
            item.classList.add('active');
            const targetViewId = item.getAttribute('data-view');
            document.getElementById(targetViewId).classList.add('active');
        });
    });

    const showNotification = (msg, type = 'success') => {
        notification.textContent = msg;
        notification.className = `notification-banner show nav-${type}`;
        setTimeout(() => notification.classList.remove('show'), 3000);
    };

    const consoleLog = (message, type = 'normal') => {
        const time = new Date().toLocaleTimeString();
        let colorClass = type === 'error' ? 'log-error' : (type === 'success' ? 'log-success' : '');
        consoleTerminal.innerHTML += `<div class="log-entry ${colorClass}"><span class="log-time">[${time}]</span> ${message}</div>`;
        consoleTerminal.scrollTop = consoleTerminal.scrollHeight;
    };

    // -------------------------------
    // PERSISTENCE FUNCTION (ADDED)
    // -------------------------------
    const persistFS = () => {
        if (typeof FS !== "undefined") {
            FS.syncfs(false, function (err) {
                if (err) console.error("FS sync failed:", err);
            });
        }
    };

    // --- C Wrapper API Helpers ---
    
    const fetchInventory = () => {
        const c_string_ptr = Module.ccall('wasm_getAllProducts', 'number', [], []);
        const rawString = UTF8ToString(c_string_ptr);
        
        const products = [];
        if (!rawString) return products;

        const lines = rawString.trim().split('\n');
        lines.forEach(line => {
            if(!line) return;
            const parts = line.split('|');
            products.push({
                id: parseInt(parts[0]),
                name: parts[1],
                price: parseFloat(parts[2]),
                quantity: parseInt(parts[3])
            });
        });
        return products;
    };

    const renderDashboard = () => {
        const products = fetchInventory();
        const tbody = document.getElementById('inventory-tbody');
        const emptyState = document.getElementById('empty-state');
        const lowStockList = document.getElementById('low-stock-list');
        const totalValueDisplay = document.getElementById('total-value-display');
        
        tbody.innerHTML = '';
        lowStockList.innerHTML = '';
        let totalValue = 0;
        let lowStockCount = 0;

        if (products.length === 0) {
            emptyState.classList.remove('hidden');
            document.querySelector('.table-container').classList.add('hidden');
            totalValueDisplay.textContent = '$0.00';
            lowStockList.innerHTML = `<li class="alert-item success">Inventory is empty.</li>`;
            return;
        }

        emptyState.classList.add('hidden');
        document.querySelector('.table-container').classList.remove('hidden');

        products.forEach(item => {
            const isLow = item.quantity < 5;
            tbody.innerHTML += `
                <tr>
                    <td>#${item.id}</td>
                    <td class="product-name">${item.name}</td>
                    <td>$${item.price.toFixed(2)}</td>
                    <td>${item.quantity}</td>
                    <td><span class="badge ${isLow ? 'badge-warning' : 'badge-success'}">${isLow ? 'Low Stock' : 'In Stock'}</span></td>
                </tr>
            `;

            if(isLow) {
                lowStockCount++;
                lowStockList.innerHTML += `
                    <li class="alert-item warning">⚠️ ${item.name} (ID: ${item.id}) - Only ${item.quantity} left!</li>
                `;
            }
            totalValue += (item.price * item.quantity);
        });

        if (lowStockCount === 0) lowStockList.innerHTML = `<li class="alert-item success">All stock levels are optimal.</li>`;
        totalValueDisplay.textContent = `$${totalValue.toLocaleString('en-US', {minimumFractionDigits: 2})}`;
    };

    // --- WebAssembly Init ---
    window.Module = window.Module || {};
    window.Module.onRuntimeInitialized = () => {
        setTimeout(() => {
            overlay.classList.remove('active');
            mainApp.style.display = 'flex';
            consoleLog("WASM engine mounted successfully.", "success");

            if (typeof FS !== "undefined") {
                FS.syncfs(true, function (err) {
                    consoleLog("Persistent inventory loaded.");
                    renderDashboard();
                });
            } else {
                renderDashboard();
            }

        }, 1000);
    };

    // --- Form Handlers calling C functions ---

    document.getElementById('form-add').addEventListener('submit', (e) => {
        e.preventDefault();
        const id = parseInt(document.getElementById('add-id').value);
        const name = document.getElementById('add-name').value;
        const price = parseFloat(document.getElementById('add-price').value);
        const qty = parseInt(document.getElementById('add-qty').value);

        const res = Module.ccall('wasm_addProduct', 'number', 
            ['number', 'string', 'number', 'number'], 
            [id, name, price, qty]
        );

        if (res === 1) {
            showNotification('Product Added Successfully');
            consoleLog(`Added ID ${id}: ${name}`, 'success');
            renderDashboard();
            persistFS();   // ADDED
            e.target.reset();
        } else if (res === 0) {
            showNotification('Error: ID already exists', 'error');
            consoleLog(`Add Failed: ID ${id} is a duplicate`, 'error');
        } else {
            showNotification('File Error', 'error');
        }
    });

    document.getElementById('form-search').addEventListener('submit', (e) => {
        e.preventDefault();
        const id = parseInt(document.getElementById('search-id').value);
        const products = fetchInventory();
        const item = products.find(p => p.id === id);
        
        const resultsContainer = document.getElementById('search-results');
        resultsContainer.classList.remove('hidden');
        
        if (item) {
            resultsContainer.innerHTML = `
                <div class="search-card success">
                    <h4>Product Found</h4>
                    <p><strong>ID:</strong> ${item.id}</p>
                    <p><strong>Name:</strong> ${item.name}</p>
                    <p><strong>Price:</strong> $${item.price.toFixed(2)}</p>
                    <p><strong>Stock:</strong> ${item.quantity}</p>
                </div>
            `;
            consoleLog(`Searched for ID: ${id} - Found.`);
        } else {
            resultsContainer.innerHTML = `
                <div class="search-card error">
                    <h4>Product Not Found</h4>
                    <p>No item exists with ID ${id}</p>
                </div>
            `;
            consoleLog(`Searched for ID: ${id} - Not Found.`, 'error');
        }
    });

    document.getElementById('form-update').addEventListener('submit', (e) => {
        e.preventDefault();
        const id = parseInt(document.getElementById('update-id').value);
        const isBuy = parseInt(document.querySelector('input[name="update-type"]:checked').value) === 1 ? 1 : 0;
        const qty = parseInt(document.getElementById('update-qty').value);

        const res = Module.ccall('wasm_updateStock', 'number', ['number', 'number', 'number'], [id, isBuy, qty]);

        if (res === 1) {
            showNotification(`Stock ${isBuy ? 'added' : 'deducted'} successfully`);
            consoleLog(`Updated ID ${id} stock by ${qty}`, 'success');
            renderDashboard();
            persistFS();   // ADDED
            e.target.reset();
        } else if (res === -2) {
            showNotification('Error: Not enough stock for sale', 'error');
            consoleLog(`Update Failed: Insufficient stock for ID ${id}`, 'error');
        } else {
            showNotification('Error: Product Not Found', 'error');
            consoleLog(`Update Failed: ID ${id} not found`, 'error');
        }
    });

    document.getElementById('form-delete').addEventListener('submit', (e) => {
        e.preventDefault();
        const id = parseInt(document.getElementById('delete-id').value);
        
        const res = Module.ccall('wasm_deleteProduct', 'number', ['number'], [id]);

        if (res === 1) {
            showNotification('Product deleted completely');
            consoleLog(`Deleted ID ${id}`, 'success');
            renderDashboard();
            persistFS();   // ADDED
            e.target.reset();
            document.querySelector('[data-view="dashboard"]').click();
        } else {
            showNotification('Error: Product Not Found', 'error');
            consoleLog(`Delete Failed: ID ${id} not found`, 'error');
        }
    });

    document.getElementById('btn-refresh-inventory').addEventListener('click', () => {
        renderDashboard();
        showNotification('Data refreshed');
    });

    document.getElementById('btn-clear-log').addEventListener('click', () => {
        consoleTerminal.innerHTML = '';
    });
});
