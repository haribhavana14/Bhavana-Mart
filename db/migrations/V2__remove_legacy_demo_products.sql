-- V2: Remove legacy demo products that have no valid seller
DELETE FROM products
WHERE seller_id = 0;
