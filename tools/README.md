# DevOps Toolchain Setup

This directory contains the complete DevOps toolchain configuration for the L-Band Splitter project using Docker Compose.

## Services Included

- **GitLab CE** - Git repository hosting and CI/CD
- **Jenkins** - Build automation and CI/CD pipelines  
- **SonarQube** - Static code analysis and quality gates
- **Jira** - Issue tracking and project management
- **Confluence** - Documentation and knowledge base
- **InfluxDB** - Time series database for metrics and monitoring
- **Grafana Loki** - Log aggregation and storage system
- **Grafana** - Monitoring dashboards and observability platform
- **PostgreSQL** - Database backend for services
- **Redis** - Caching layer
- **Nginx** - Reverse proxy and load balancer

## Prerequisites

- Docker Engine 20.10+
- Docker Compose 2.0+
- At least 8GB RAM available
- At least 50GB disk space

## Quick Start

1. **Start the toolchain:**
   ```bash
   docker-compose -f docker-compose.devops.yml up -d
   ```

2. **Add hostnames to your `/etc/hosts` file:**
   ```
   127.0.0.1 gitlab.splitter.local
   127.0.0.1 jenkins.splitter.local  
   127.0.0.1 sonarqube.splitter.local
   127.0.0.1 jira.splitter.local
   127.0.0.1 confluence.splitter.local
   127.0.0.1 grafana.splitter.local
   127.0.0.1 influxdb.splitter.local
   ```

3. **Access the services:**
   - **GitLab**: http://gitlab.splitter.local or http://localhost:8080
   - **Jenkins**: http://jenkins.splitter.local or http://localhost:8081
   - **SonarQube**: http://sonarqube.splitter.local or http://localhost:9000
   - **Jira**: http://jira.splitter.local or http://localhost:8082
   - **Confluence**: http://confluence.splitter.local or http://localhost:8083
   - **Grafana**: http://grafana.splitter.local or http://localhost:3000
   - **InfluxDB**: http://influxdb.splitter.local or http://localhost:8086

## Service Configuration

⚠️ **For service credentials and login information, see `tools/CREDENTIALS.md` (not tracked in git for security)**

### GitLab
- **SSH access**: port 2222
- **Admin interface**: Available after initial setup

### Jenkins
- **Plugins**: Pre-configured with essential CI/CD plugins
- **Agent port**: 50000

### SonarQube
- **Database**: PostgreSQL backend
- **Community edition** with standard analyzers

### Jira
- **Setup required**: Complete setup wizard on first access
- **Database**: PostgreSQL backend
- **License**: Required for production use

### Confluence
- **Setup required**: Complete setup wizard on first access  
- **Database**: PostgreSQL backend
- **License**: Required for production use

### InfluxDB
- **Organization**: splitter
- **Default bucket**: metrics
- **Retention**: 30 days

### Grafana
- **Pre-configured datasources**: InfluxDB, Loki, PostgreSQL
- **Default dashboards**: DevOps overview dashboard included

### Loki
- **Port**: 3100
- **Storage**: Local filesystem
- **Log retention**: Based on disk space available

## Database Configuration

PostgreSQL instance provides databases for:
- `sonarqube` - SonarQube data (user: sonar)
- `jiradb` - Jira data (user: jira)
- `confluencedb` - Confluence data (user: confluence)

## File Structure

```
tools/
├── README.md
├── CREDENTIALS.md           # Service credentials (not tracked in git)
├── jenkins/
│   ├── plugins.txt           # Jenkins plugin list
│   └── init.groovy.d/        # Jenkins initialization scripts
│       ├── 01-security.groovy
│       └── 02-configure-tools.groovy
├── nginx/
│   └── nginx.conf           # Reverse proxy configuration
├── postgres/
│   └── init-multiple-databases.sh  # Database initialization
├── loki/
│   └── loki-config.yaml     # Loki configuration
└── grafana/
    ├── provisioning/        # Grafana provisioning configs
    │   ├── datasources/     # Datasource configurations
    │   └── dashboards/      # Dashboard provisioning
    └── dashboards/          # Dashboard definitions
        └── devops-overview.json
```

## Volumes and Data Persistence

All service data is persisted in Docker volumes:
- `gitlab_data`, `gitlab_logs`, `gitlab_config`
- `jenkins_home`
- `sonarqube_data`, `sonarqube_logs`, `sonarqube_extensions`
- `postgres_data`
- `jira_data`
- `confluence_data`
- `redis_data`
- `influxdb_data`, `influxdb_config`
- `loki_data`
- `grafana_data`

## Networking

Services communicate via the `devops_network` bridge network (172.20.0.0/16).

## Health Checks

All services include health checks for monitoring:
- GitLab: GitLab status command
- Jenkins: HTTP endpoint check
- SonarQube: API status endpoint
- PostgreSQL: Database connection test
- Others: Standard HTTP health endpoints

## Resource Requirements

**Minimum system requirements:**
- CPU: 4 cores
- RAM: 8GB
- Disk: 50GB

**Recommended for production:**
- CPU: 8+ cores  
- RAM: 16GB+
- Disk: 200GB+

## Troubleshooting

### Common Issues

1. **Services not starting**: Check available memory and disk space
2. **Database connection issues**: Ensure PostgreSQL is healthy before dependent services
3. **Proxy issues**: Verify nginx configuration and service names

### Useful Commands

```bash
# Check service status
docker-compose -f docker-compose.devops.yml ps

# View service logs
docker-compose -f docker-compose.devops.yml logs <service-name>

# Restart specific service
docker-compose -f docker-compose.devops.yml restart <service-name>

# Stop all services
docker-compose -f docker-compose.devops.yml down

# Remove all data (CAUTION!)
docker-compose -f docker-compose.devops.yml down -v
```

## Production Considerations

- **Security**: Change default passwords and configure proper authentication
- **SSL/TLS**: Configure HTTPS certificates for production deployment
- **Backup**: Implement regular backup strategy for persistent volumes
- **Monitoring**: Add monitoring solutions (Prometheus/Grafana)
- **Scaling**: Consider resource limits and horizontal scaling for high load

## Integration Workflow

1. **Code Development** → GitLab repositories
2. **CI/CD Pipelines** → Jenkins builds triggered by GitLab webhooks
3. **Quality Gates** → SonarQube analysis integrated in pipelines
4. **Issue Tracking** → Jira for bugs, features, and project management
5. **Documentation** → Confluence for project documentation and runbooks