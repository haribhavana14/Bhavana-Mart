#include "productrepository.h"
#include <sqlite3.h>

static sqlite3* db = nullptr;

ProductRepository::ProductRepository() {
    sqlite3_open("database/bhavanamart.db", &db);
    createTable();
}

void ProductRepository::createTable() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS products ("
        "id INTEGER PRIMARY KEY,"
        "name TEXT NOT NULL,"
        "price REAL NOT NULL,"
        "stock INTEGER NOT NULL);";

    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}

void ProductRepository::addProduct(const Product& product) {
    const char* sql =
        "INSERT OR REPLACE INTO products "
        "(id, name, price, stock) VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, product.id);
    sqlite3_bind_text(stmt, 2, product.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, product.price);
    sqlite3_bind_int(stmt, 4, product.stock);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<Product> ProductRepository::getProducts() {
    std::vector<Product> products;

    const char* sql = "SELECT id, name, price, stock FROM products;";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Product p;
        p.id = sqlite3_column_int(stmt, 0);
        p.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        p.price = sqlite3_column_double(stmt, 2);
        p.stock = sqlite3_column_int(stmt, 3);
        products.push_back(p);
    }

    sqlite3_finalize(stmt);
    return products;
}