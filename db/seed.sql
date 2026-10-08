-- BhavanaMart seed data
-- ADMIN is seeded; ADMIN registration is not allowed.

INSERT INTO users (name, email, password_hash, role)
VALUES (
    'BhavanaMart Admin',
    'admin@bhavanamart.local',
    '$argon2id$v=19$m=65536,t=2,p=1$2ugNdX0eifrsMkiJm/vh2w$XqnAQTFiSHtAKvPliMR+RpetjB5mu84pe7z3yzWPbSw',
    'ADMIN'
)
ON CONFLICT (email) DO NOTHING;
