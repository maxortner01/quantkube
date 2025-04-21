CREATE TABLE IF NOT EXISTS companies (id SERIAL PRIMARY KEY, name TEXT NOT NULL UNIQUE);

CREATE TABLE IF NOT EXISTS price_data (time TIMESTAMPTZ NOT NULL, company_id INTEGER NOT NULL REFERENCES companies(id), price NUMERIC NOT NULL, PRIMARY KEY (time, company_id));

-- Convert to a hypertable
SELECT create_hypertable('price_data', 'time', if_not_exists => TRUE);

-- Create an index
CREATE INDEX IF NOT EXISTS idx_price_data_company_time ON price_data (company_id, time DESC);

-- Compress
ALTER TABLE price_data SET (timescaledb.compress, timescaledb.compress_segmentby = 'company_id');