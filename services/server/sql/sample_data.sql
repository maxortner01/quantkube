INSERT INTO companies (name) VALUES ('Acme Corp') ON CONFLICT (name) DO NOTHING;

INSERT INTO price_data (time, company_id, price) VALUES (NOW(), 1, 102.5);