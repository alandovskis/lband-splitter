#!/bin/bash
set -e

# Initialize multiple databases and users for PostgreSQL
function create_user_and_database() {
    local database=$1
    local user=$2
    local password=$3
    
    echo "Creating user '$user' and database '$database'"
    psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname "$POSTGRES_DB" <<-EOSQL
        CREATE USER $user WITH PASSWORD '$password';
        CREATE DATABASE $database OWNER $user;
        GRANT ALL PRIVILEGES ON DATABASE $database TO $user;
EOSQL
}

# Wait for PostgreSQL to be ready
until pg_isready -h localhost -p 5432 -U "$POSTGRES_USER"
do
    echo "Waiting for PostgreSQL to be ready..."
    sleep 2
done

# Create databases and users
create_user_and_database "jiradb" "jira" "jira123"
create_user_and_database "confluencedb" "confluence" "confluence123"

echo "Multiple databases and users created successfully"