# 💜 Bhavana-Mart

A native C++ e-commerce marketplace application built using **Drogon**, **SQLite**, and **HTML/CSS/JavaScript**.

## 🔗 Live Application

**https://bhavana-mart.onrender.com**

## 📌 Project Overview

Bhavana-Mart is a multi-category e-commerce marketplace designed to provide a simple shopping experience through a web-based interface with a native C++ backend.

The project follows the technology and architecture direction specified in the C++ Capstone Project requirements.

## 🛠️ Technology Stack

* **Backend:** C++20
* **Web Framework:** Drogon
* **Database:** SQLite
* **Frontend:** HTML, CSS, Vanilla JavaScript
* **Build System:** CMake + Ninja
* **Dependency Management:** vcpkg
* **Deployment:** Docker + Render
* **Version Control:** Git + GitHub

## 🏗️ Project Structure

```text
BhavanaMart/
├── src/
│   ├── main.cpp
│   └── index.html
├── models/
│   └── product.h
├── repositories/
│   ├── productrepository.h
│   └── productrepository.cpp
├── database/
├── CMakeLists.txt
├── Dockerfile
└── README.md
```

## ✨ Current Features

### 🛍️ E-Commerce UI

* Purple-themed modern shopping interface
* Product cards with images
* Multiple product categories
* Product search
* Add-to-cart counter
* Responsive layout

### 📂 Categories

* Clothing
* Jewellery
* Shoes
* Electronics
* Beauty
* Home
* Accessories

### ⚙️ Backend

* Native C++ backend using Drogon
* REST-style product APIs
* SQLite database integration
* Product insertion and retrieval
* Database health checking

## 🔌 API Endpoints

### Health Check

```text
GET /api/v1/health
```

Example response:

```json
{
  "status": "UP",
  "db": "UP"
}
```

### Get Products

```text
GET /api/v1/products
```

### Add Product

```text
POST /api/v1/products
```

Example request:

```json
{
  "id": 6,
  "name": "Smart Watch",
  "price": 2499,
  "stock": 10
}
```

## 🗄️ Demo Products

The current demo database contains products such as:

* Laptop
* Mouse
* Keyboard
* Headphones
* Monitor

The frontend also contains additional category-based demo products for the shopping interface.

## 🚀 Run Locally

### Build

```powershell
cmake --build build
```

### Run

```powershell
$env:Path="$PWD\vcpkg_installed\x64-mingw-dynamic\bin;$env:Path"
.\build\BhavanaMart.exe
```

Open:

```text
http://127.0.0.1:8080
```

## 🌐 Deployment

The application is containerized using Docker and deployed on Render.

Live URL:

```text
https://bhavana-mart.onrender.com
```

## 📚 Capstone Alignment

The project is being developed according to the provided C++ Capstone Project specification, including the planned marketplace modules such as authentication, product management, cart, checkout, orders, administration, and reviews.

The current deployment represents a working MVP/base implementation, with additional capstone features to be developed and integrated in subsequent stages.

## 👩‍💻 Project

**Bhavana-Mart**
Native C++ E-Commerce Marketplace
**B.Tech Information Technology**

## 📄 License

This project is developed for academic/educational purposes.
