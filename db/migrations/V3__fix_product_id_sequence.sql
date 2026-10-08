-- V3: Ensure product IDs are auto-generated

CREATE SEQUENCE IF NOT EXISTS products_id_seq;

ALTER TABLE products
ALTER COLUMN id SET DEFAULT nextval('products_id_seq');

ALTER SEQUENCE products_id_seq
OWNED BY products.id;

SELECT setval(
    'products_id_seq',
    COALESCE((SELECT MAX(id) FROM products), 0) + 1,
    false
);
