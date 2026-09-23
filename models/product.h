#pragma once

#include <string>

#include "money.h"

struct Product
{
    int id{0};
    int seller_id{0};

    std::string name;
    std::string description;

    Money price;

    int stock_qty{0};

    std::string category;
    std::string image_url;
};