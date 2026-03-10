#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Allows functions to be exported to frontend JavaScript when compiled to WebAssembly */
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#define FILE_NAME "inventory.dat"
#define LOW_STOCK 5
#define MAX_NAME_LEN 50

struct Product {
    int id;
    char name[MAX_NAME_LEN];
    float price;
    int quantity;
};

/* Function Prototypes */
void addProduct();
void displayProducts();
void searchProduct();
void updateStock();
void deleteProduct();
void lowStockReport();
void totalInventoryValue();
int checkDuplicateID(int id);

/* ========================================================= 
   INPUT VALIDATION HELPERS (Handles Extreme Test Cases)
   ========================================================= */

/* Flushes the standard input buffer to prevent infinite loop bugs on wrong inputs */
void cleanBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* Safely reads integers and rejects all strings, chars, or empty inputs */
int getValidInt(const char* prompt) {
    int value;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &value) == 1) {
            cleanBuffer();
            return value;
        } else {
            printf("  [!] Error: Invalid type. Please enter a valid number.\n");
            cleanBuffer();
        }
    }
}

/* Safely reads floats/prices */
float getValidFloat(const char* prompt) {
    float value;
    while (1) {
        printf("%s", prompt);
        if (scanf("%f", &value) == 1) {
            cleanBuffer();
            return value;
        } else {
            printf("  [!] Error: Invalid type. Please enter a valid decimal number.\n");
            cleanBuffer();
        }
    }
}

/* Safely reads strings preventing buffer overflow and rejecting empty/whitespace inputs */
void getValidString(const char* prompt, char* buffer, int max_len) {
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, max_len, stdin) != NULL) {
            size_t len = strlen(buffer);
            
            // Remove trailing newline
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
                len--;
            } else if (len == (size_t)(max_len - 1)) {
                cleanBuffer(); // Flush remainder if buffer limit is hit
            }

            // Ensure not strictly whitespaces
            int isEmpty = 1;
            for (size_t i = 0; i < len; i++) {
                if (!isspace((unsigned char)buffer[i])) {
                    isEmpty = 0;
                    break;
                }
            }

            if (!isEmpty) {
                return;
            } else {
                printf("  [!] Error: Field cannot be empty.\n");
            }
        }
    }
}

/* ========================================================= 
   MAIN FUNCTION (Backend Interactive Terminal)
   ========================================================= */

int main()
{
#ifndef __EMSCRIPTEN__
    int choice;
    while(1)
    {
        printf("\n==========================================\n");
        printf("    INVENTORY MANAGEMENT SYSTEM ADMIN\n");
        printf("==========================================\n");
        printf("  1. Add Product\n");
        printf("  2. Display Products\n");
        printf("  3. Search Product\n");
        printf("  4. Update Stock\n");
        printf("  5. Delete Product\n");
        printf("  6. Low Stock Report\n");
        printf("  7. Total Inventory Value\n");
        printf("  8. Exit\n");
        printf("------------------------------------------\n");

        choice = getValidInt("  Enter your choice (1-8): ");
        printf("\n");

        switch(choice)
        {
            case 1: addProduct(); break;
            case 2: displayProducts(); break;
            case 3: searchProduct(); break;
            case 4: updateStock(); break;
            case 5: deleteProduct(); break;
            case 6: lowStockReport(); break;
            case 7: totalInventoryValue(); break;
            case 8: 
                printf("  Exiting System... Goodbye!\n");
                exit(0);
            default:
                printf("  [!] Invalid choice! Please select between 1 and 8.\n");
        }
    }
#endif
    return 0;
}

/* ========================================================= 
   CORE BUSINESS LOGIC
   ========================================================= */

/* CHECK DUPLICATE PRODUCT ID */
EMSCRIPTEN_KEEPALIVE 
int checkDuplicateID(int id)
{
    struct Product p;
    FILE *fp = fopen(FILE_NAME, "rb");
    if(fp == NULL) return 0;

    while(fread(&p, sizeof(p), 1, fp))
    {
        if(p.id == id)
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

/* ADD PRODUCT */
void addProduct()
{
    struct Product p;
    FILE *fp = fopen(FILE_NAME, "ab"); // Append Binary
    if(fp == NULL)
    {
        printf("  [!] File error! Could not connect to database.\n");
        return;
    }

    printf("--- ADD NEW PRODUCT ---\n");
    while(1) {
        p.id = getValidInt("Enter Product ID (> 0): ");
        if (p.id <= 0) {
            printf("  [!] Error: Product ID must be positive.\n");
            continue;
        }
        if(checkDuplicateID(p.id)) {
            printf("  [!] Error: Product ID %d already exists!\n", p.id);
            continue;
        }
        break; // Passed
    }

    getValidString("Enter Product Name: ", p.name, sizeof(p.name));

    while(1) {
        p.price = getValidFloat("Enter Price ($): ");
        if(p.price < 0) {
            printf("  [!] Error: Price cannot be negative!\n");
        } else {
            break; // Passed
        }
    }

    while(1) {
        p.quantity = getValidInt("Enter Initial Quantity: ");
        if(p.quantity < 0) {
            printf("  [!] Error: Quantity cannot be negative!\n");
        } else {
            break; // Passed
        }
    }

    fwrite(&p, sizeof(p), 1, fp);
    fclose(fp);
    printf("  [+] Product Added Successfully.\n");
}

/* DISPLAY PRODUCTS */
void displayProducts()
{
    struct Product p;
    FILE *fp = fopen(FILE_NAME, "rb");
    
    if(fp == NULL) {
        printf("  [!] No records found. Database is currently empty.\n");
        return;
    }

    // Check if empty
    fseek(fp, 0, SEEK_END);
    if(ftell(fp) == 0) {
        printf("  [!] Inventory is currently empty.\n");
        fclose(fp);
        return;
    }
    rewind(fp);

    printf("--- ALL PRODUCTS ---\n");
    printf("%-8s | %-25s | %-10s | %-10s\n", "ID", "Product Name", "Price($)", "Quantity");
    printf("------------------------------------------------------------------\n");

    while(fread(&p, sizeof(p), 1, fp))
    {
        printf("%-8d | %-25s | $%-9.2f | %-10d\n", p.id, p.name, p.price, p.quantity);
    }
    printf("------------------------------------------------------------------\n");
    fclose(fp);
}

/* SEARCH PRODUCT */
void searchProduct()
{
    int id, found = 0;
    struct Product p;
    FILE *fp = fopen(FILE_NAME, "rb");
    if(fp == NULL) {
        printf("  [!] No records found.\n");
        return;
    }

    printf("--- SEARCH PRODUCT ---\n");
    id = getValidInt("Enter Product ID to search: ");

    while(fread(&p, sizeof(p), 1, fp))
    {
        if(p.id == id)
        {
            printf("\n  [+] Product Found:\n");
            printf("      ID      : %d\n", p.id);
            printf("      Name    : %s\n", p.name);
            printf("      Price   : $%.2f\n", p.price);
            printf("      Stock   : %d\n", p.quantity);
            found = 1;
            break;
        }
    }

    if(!found) printf("  [!] Product with ID %d not found.\n", id);
    fclose(fp);
}

/* UPDATE STOCK */
void updateStock()
{
    int id, choice, qty;
    struct Product p;
    int found = 0;
    
    FILE *fp = fopen(FILE_NAME, "rb+");
    if(fp == NULL) {
        printf("  [!] No records found.\n");
        return;
    }

    printf("--- UPDATE STOCK ---\n");
    id = getValidInt("Enter Product ID to update: ");

    while(fread(&p, sizeof(p), 1, fp))
    {
        if(p.id == id)
        {
            found = 1;
            printf("\n  Current Stock for '%s': %d\n", p.name, p.quantity);
            printf("  1. Purchase (Add Stock)\n");
            printf("  2. Sale (Reduce Stock)\n");
            
            while(1) {
                choice = getValidInt("  Select operation (1 or 2): ");
                if (choice == 1 || choice == 2) break;
                printf("  [!] Error: Invalid choice.\n");
            }

            while(1) {
                qty = getValidInt("Enter quantity: ");
                if(qty < 0) {
                    printf("  [!] Error: Quantity cannot be negative.\n");
                    continue;
                }
                if (qty == 0) {
                    printf("  [-] No changes made.\n");
                    fclose(fp);
                    return;
                }

                if(choice == 1) { // Purchase
                    p.quantity += qty;
                    break;
                } else if(choice == 2) { // Sale
                    if(qty > p.quantity) {
                        printf("  [!] Not enough stock! Maximum you can sell is %d.\n", p.quantity);
                    } else {
                        p.quantity -= qty;
                        break;
                    }
                }
            }

            // Write updated record back to file correctly
            fseek(fp, -(long)sizeof(p), SEEK_CUR);
            fwrite(&p, sizeof(p), 1, fp);
            printf("  [+] Stock Updated Successfully. New Stock for %s: %d\n", p.name, p.quantity);
            break;
        }
    }

    if(!found) printf("  [!] Product not found.\n");
    fclose(fp);
}

/* DELETE PRODUCT */
void deleteProduct()
{
    int id, found = 0;
    struct Product p;
    
    FILE *fp = fopen(FILE_NAME, "rb");
    if(fp == NULL) {
        printf("  [!] No records found.\n");
        return;
    }

    FILE *temp = fopen("temp.dat", "wb");
    if(temp == NULL) {
        printf("  [!] Error creating temporary manipulation file.\n");
        fclose(fp);
        return;
    }

    printf("--- DELETE PRODUCT ---\n");
    id = getValidInt("Enter Product ID to delete: ");

    while(fread(&p, sizeof(p), 1, fp))
    {
        if(p.id != id) {
            fwrite(&p, sizeof(p), 1, temp);
        } else {
            found = 1;
            printf("  [-] Removing '%s' from inventory...\n", p.name);
        }
    }

    fclose(fp);
    fclose(temp);

    // Safely replace old file with updated list
    if(found) {
        remove(FILE_NAME);
        rename("temp.dat", FILE_NAME);
        printf("  [+] Product Deleted Successfully.\n");
    } else {
        remove("temp.dat");
        printf("  [!] Product with ID %d not found.\n", id);
    }
}

/* LOW STOCK REPORT */
void lowStockReport()
{
    struct Product p;
    int found = 0;
    
    FILE *fp = fopen(FILE_NAME, "rb");
    if(fp == NULL) {
        printf("  [!] No records found.\n");
        return;
    }

    printf("--- LOW STOCK ALERTS (Qty < %d) ---\n", LOW_STOCK);
    printf("%-8s | %-25s | %-10s\n", "ID", "Product Name", "Quantity");
    printf("-------------------------------------------------\n");

    while(fread(&p, sizeof(p), 1, fp))
    {
        if(p.quantity < LOW_STOCK)
        {
            printf("%-8d | %-25s | %-10d\n", p.id, p.name, p.quantity);
            found = 1;
        }
    }

    if(!found) {
        printf("  [+] All products have adequate stock.\n");
    }
    printf("-------------------------------------------------\n");

    fclose(fp);
}

/* TOTAL INVENTORY VALUE */
void totalInventoryValue()
{
    struct Product p;
    float total = 0;
    
    FILE *fp = fopen(FILE_NAME, "rb");
    if(fp == NULL) {
        printf("  [!] No records found.\n");
        return;
    }

    while(fread(&p, sizeof(p), 1, fp))
    {
        total += (p.price * p.quantity);
    }

    printf("--- VALUATION ---\n");
    printf("  Total Inventory Portfolio Value: $%.2f\n", total);

    fclose(fp);
}

/* ========================================================= 
   WEBASSEMBLY API (Simplified Integration)
   ========================================================= */
#ifdef __EMSCRIPTEN__

// Returns string of all products formatted as "id|name|price|qty\n"
EMSCRIPTEN_KEEPALIVE
char* wasm_getAllProducts() {
    static char buffer[10000]; // Static buffer for simple returns
    buffer[0] = '\0';
    
    FILE *fp = fopen(FILE_NAME, "rb");
    if(!fp) return buffer;
    
    struct Product p;
    while(fread(&p, sizeof(p), 1, fp)) {
        char line[200];
        snprintf(line, sizeof(line), "%d|%s|%.2f|%d\n", p.id, p.name, p.price, p.quantity);
        strcat(buffer, line);
    }
    fclose(fp);
    return buffer;
}

// Add product: returns 1 success, 0 exists, -1 error
EMSCRIPTEN_KEEPALIVE
int wasm_addProduct(int id, const char* name, float price, int quantity) {
    if (checkDuplicateID(id)) return 0;
    
    FILE *fp = fopen(FILE_NAME, "ab");
    if (!fp) return -1;
    
    struct Product p;
    p.id = id;
    strncpy(p.name, name, sizeof(p.name)-1);
    p.name[sizeof(p.name)-1] = '\0';
    p.price = price;
    p.quantity = quantity;
    
    fwrite(&p, sizeof(p), 1, fp);
    fclose(fp);
    return 1;
}

// Update stock: isPurchase (1 for add, 0 for sell). Returns 1 success, -2 not enough stock, 0 not found
EMSCRIPTEN_KEEPALIVE
int wasm_updateStock(int id, int isPurchase, int qty) {
    FILE *fp = fopen(FILE_NAME, "rb+");
    if (!fp) return 0;
    
    struct Product p;
    int found = 0;
    
    while(fread(&p, sizeof(p), 1, fp)) {
        if(p.id == id) {
            if(isPurchase) {
                p.quantity += qty;
            } else {
                if(qty > p.quantity) {
                    fclose(fp);
                    return -2; // Not enough stock
                }
                p.quantity -= qty;
            }
            fseek(fp, -(long)sizeof(p), SEEK_CUR);
            fwrite(&p, sizeof(p), 1, fp);
            found = 1;
            break;
        }
    }
    fclose(fp);
    return found ? 1 : 0;
}

// Delete product: returns 1 success, 0 not found
EMSCRIPTEN_KEEPALIVE
int wasm_deleteProduct(int id) {
    FILE *fp = fopen(FILE_NAME, "rb");
    if (!fp) return 0;
    
    FILE *temp = fopen("temp.dat", "wb");
    if (!temp) { fclose(fp); return 0; }
    
    struct Product p;
    int found = 0;
    
    while(fread(&p, sizeof(p), 1, fp)) {
        if(p.id != id) {
            fwrite(&p, sizeof(p), 1, temp);
        } else {
            found = 1;
        }
    }
    
    fclose(fp);
    fclose(temp);
    
    if(found) {
        remove(FILE_NAME);
        rename("temp.dat", FILE_NAME);
        return 1;
    } else {
        remove("temp.dat");
        return 0;
    }
}

#endif

