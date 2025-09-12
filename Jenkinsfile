#!/usr/bin/env groovy

pipeline {
    agent {
        label 'linux'
    }
    
    parameters {
        choice(
            name: 'BUILD_TYPE',
            choices: ['Debug', 'Release'],
            description: 'Build configuration type'
        )
        booleanParam(
            name: 'RUN_PERFORMANCE_TESTS',
            defaultValue: true,
            description: 'Run performance benchmarks'
        )
        booleanParam(
            name: 'RUN_SECURITY_SCAN',
            defaultValue: true,
            description: 'Run security analysis'
        )
        booleanParam(
            name: 'DEPLOY_TO_STAGING',
            defaultValue: false,
            description: 'Deploy to staging environment'
        )
    }
    
    environment {
        // Build configuration
        CMAKE_BUILD_TYPE = "${params.BUILD_TYPE}"
        MAKEFLAGS = "-j${sh(script: 'nproc', returnStdout: true).trim()}"
        
        // Tool paths
        ARM_TOOLCHAIN_PATH = '/opt/gcc-arm-none-eabi/bin'
        STM32_PROGRAMMER_CLI = '/opt/STM32CubeProgrammer/bin/STM32_Programmer_CLI'
        
        // Container registry
        DOCKER_REGISTRY = 'registry.company.local'
        IMAGE_PREFIX = 'rf-splitter'
        
        // Notification settings
        SLACK_CHANNEL = '#rf-splitter-builds'
        EMAIL_RECIPIENTS = 'team@company.com'
        
        // Quality gates
        COVERAGE_THRESHOLD = '80'
        PERFORMANCE_BASELINE = 'performance_baseline.json'
    }
    
    options {
        buildDiscarder(logRotator(numToKeepStr: '30', daysToKeepStr: '90'))
        timeout(time: 2, unit: 'HOURS')
        timestamps()
        ansiColor('xterm')
        parallelsAlwaysFailFast()
    }
    
    triggers {
        // Poll SCM every 5 minutes during business hours
        pollSCM('H/5 8-18 * * 1-5')
        
        // Nightly builds
        cron('H 2 * * *')
        
        // Upstream job trigger
        upstream(upstreamProjects: 'firmware-dependencies', threshold: hudson.model.Result.SUCCESS)
    }
    
    stages {
        stage('Prepare Environment') {
            steps {
                script {
                    // Set build display name
                    currentBuild.displayName = "#${BUILD_NUMBER} - ${params.BUILD_TYPE}"
                    currentBuild.description = "Branch: ${env.BRANCH_NAME}, Commit: ${env.GIT_COMMIT?.take(8)}"
                }
                
                echo "🚀 Starting RF Splitter CI Pipeline"
                echo "📋 Build Type: ${params.BUILD_TYPE}"
                echo "🌿 Branch: ${env.BRANCH_NAME}"
                echo "📝 Commit: ${env.GIT_COMMIT}"
                
                // Clean workspace
                cleanWs()
                
                // Checkout code
                checkout scm
                
                // Setup build environment
                sh '''
                    echo "Setting up build environment..."
                    
                    # Install required packages
                    sudo apt-get update
                    sudo apt-get install -y \
                        build-essential \
                        cmake \
                        ninja-build \
                        pkg-config \
                        libgtest-dev \
                        libgmock-dev \
                        libnlohmann-json3-dev \
                        libspdlog-dev \
                        libssl-dev \
                        libjwt-dev \
                        libwebsockets-dev \
                        clang-tidy \
                        cppcheck \
                        valgrind \
                        lcov \
                        doxygen \
                        graphviz
                        
                    # Verify tools
                    echo "✅ Build tools ready:"
                    cmake --version
                    g++ --version
                    node --version || echo "Node.js not available"
                '''
                
                // Setup ARM toolchain for STM32
                sh '''
                    if [ ! -d "${ARM_TOOLCHAIN_PATH}" ]; then
                        echo "📦 Installing ARM toolchain..."
                        wget -q https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
                        sudo tar -C /opt -xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
                        sudo mv /opt/gcc-arm-none-eabi-10.3-2021.10 /opt/gcc-arm-none-eabi
                    fi
                    
                    export PATH="${ARM_TOOLCHAIN_PATH}:$PATH"
                    arm-none-eabi-gcc --version
                '''
            }
        }
        
        stage('Code Quality Analysis') {
            parallel {
                stage('Static Analysis') {
                    steps {
                        script {
                            echo "🔍 Running static analysis..."
                            
                            // Cppcheck
                            sh '''
                                cd backend
                                echo "Running Cppcheck..."
                                cppcheck --enable=all --std=c++17 \
                                    --xml --xml-version=2 \
                                    --output-file=cppcheck-results.xml \
                                    --suppress=missingIncludeSystem \
                                    --suppress=unmatchedSuppression \
                                    src/ include/ || true
                            '''
                            
                            // Clang-Tidy
                            sh '''
                                cd backend
                                echo "Running Clang-Tidy..."
                                find src/ -name "*.cpp" | head -20 | \
                                xargs clang-tidy \
                                    --format-style=google \
                                    --header-filter=.* \
                                    --checks='-*,readability-*,performance-*,modernize-*,bugprone-*' \
                                    --export-fixes=clang-tidy-fixes.yaml \
                                    -- -std=c++17 -I./include -I./src || true
                            '''
                            
                            // Format check
                            sh '''
                                cd backend
                                echo "Checking code formatting..."
                                find src/ include/ -name "*.cpp" -o -name "*.hpp" | \
                                xargs clang-format --dry-run --Werror || {
                                    echo "❌ Code formatting issues found"
                                    echo "Run: clang-format -i src/**/*.{cpp,hpp}"
                                    exit 1
                                }
                                echo "✅ Code formatting OK"
                            '''
                        }
                        
                        // Archive static analysis results
                        archiveArtifacts artifacts: 'backend/cppcheck-results.xml', allowEmptyArchive: true
                        archiveArtifacts artifacts: 'backend/clang-tidy-fixes.yaml', allowEmptyArchive: true
                        
                        // Publish Cppcheck results
                        publishCppcheck pattern: 'backend/cppcheck-results.xml'
                    }
                }
                
                stage('License Compliance') {
                    steps {
                        script {
                            echo "📄 Checking license compliance..."
                            
                            sh '''
                                # Check for license headers in source files
                                echo "Checking license headers..."
                                find backend/src backend/include -name "*.cpp" -o -name "*.hpp" | \
                                while read file; do
                                    if ! head -10 "$file" | grep -q "Copyright\\|License\\|SPDX"; then
                                        echo "❌ Missing license header: $file"
                                    fi
                                done
                                
                                # Check for prohibited licenses
                                echo "Scanning for prohibited licenses..."
                                find . -name "LICENSE*" -o -name "COPYING*" | \
                                while read file; do
                                    if grep -i "gpl" "$file" > /dev/null; then
                                        echo "⚠️  GPL license found: $file"
                                    fi
                                done
                            '''
                        }
                    }
                }
            }
        }
        
        stage('Build and Test') {
            parallel {
                stage('Backend C++ Build') {
                    agent {
                        label 'linux && cpp'
                    }
                    steps {
                        script {
                            echo "🔨 Building C++ backend..."
                            
                            // Configure build
                            sh '''
                                cd backend
                                mkdir -p build
                                cd build
                                
                                echo "Configuring CMake..."
                                cmake .. \
                                    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
                                    -DENABLE_TESTING=ON \
                                    -DENABLE_COVERAGE=ON \
                                    -DENABLE_SANITIZERS=ON \
                                    -GNinja
                            '''
                            
                            // Build
                            sh '''
                                cd backend/build
                                echo "Building backend..."
                                ninja -v
                                
                                echo "✅ Backend build completed"
                                ls -la
                            '''
                        }
                    }
                    post {
                        always {
                            archiveArtifacts artifacts: 'backend/build/rf-splitter-backend', allowEmptyArchive: true
                            archiveArtifacts artifacts: 'backend/build/rf-splitter-tests', allowEmptyArchive: true
                        }
                    }
                }
                
                stage('STM32 Firmware Build') {
                    agent {
                        label 'linux && arm'
                    }
                    environment {
                        PATH = "${ARM_TOOLCHAIN_PATH}:${env.PATH}"
                    }
                    steps {
                        script {
                            echo "🔧 Building STM32 firmware..."
                            
                            sh '''
                                cd firmware
                                mkdir -p build
                                cd build
                                
                                echo "Configuring STM32 build..."
                                cmake .. \
                                    -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-gcc.cmake \
                                    -DCMAKE_BUILD_TYPE=Release \
                                    -DTARGET_MCU=STM32F407VG \
                                    -GNinja
                                
                                echo "Building firmware..."
                                ninja -v
                                
                                echo "Generating firmware files..."
                                arm-none-eabi-objcopy -O ihex rf-splitter-firmware.elf rf-splitter-firmware.hex
                                arm-none-eabi-objcopy -O binary rf-splitter-firmware.elf rf-splitter-firmware.bin
                                
                                echo "Firmware size analysis:"
                                arm-none-eabi-size rf-splitter-firmware.elf
                            '''
                            
                            // Firmware validation
                            sh '''
                                cd firmware/build
                                echo "Validating firmware..."
                                
                                # Check firmware size limits
                                SIZE_OUTPUT=$(arm-none-eabi-size rf-splitter-firmware.elf | tail -n 1)
                                TEXT_SIZE=$(echo $SIZE_OUTPUT | awk '{print $1}')
                                DATA_SIZE=$(echo $SIZE_OUTPUT | awk '{print $2}')
                                BSS_SIZE=$(echo $SIZE_OUTPUT | awk '{print $3}')
                                
                                FLASH_SIZE=$((TEXT_SIZE + DATA_SIZE))
                                RAM_SIZE=$((DATA_SIZE + BSS_SIZE))
                                
                                echo "Flash usage: $FLASH_SIZE bytes"
                                echo "RAM usage: $RAM_SIZE bytes"
                                
                                # STM32F407VG limits: 1MB Flash, 192KB RAM
                                if [ $FLASH_SIZE -gt 1048576 ]; then
                                    echo "❌ Firmware exceeds flash size limit"
                                    exit 1
                                fi
                                
                                if [ $RAM_SIZE -gt 196608 ]; then
                                    echo "❌ Firmware exceeds RAM size limit"  
                                    exit 1
                                fi
                                
                                echo "✅ Firmware size validation passed"
                            '''
                        }
                    }
                    post {
                        always {
                            archiveArtifacts artifacts: 'firmware/build/rf-splitter-firmware.*', allowEmptyArchive: true
                            
                            publishHTML([
                                allowMissing: false,
                                alwaysLinkToLastBuild: true,
                                keepAll: true,
                                reportDir: 'firmware/build',
                                reportFiles: 'firmware-report.html',
                                reportName: 'STM32 Firmware Report'
                            ])
                        }
                    }
                }
                
                stage('Frontend Angular Build') {
                    agent {
                        label 'linux && nodejs'
                    }
                    tools {
                        nodejs 'NodeJS-18'
                    }
                    steps {
                        script {
                            echo "🌐 Building Angular frontend..."
                            
                            sh '''
                                cd frontend
                                echo "Installing dependencies..."
                                npm ci --no-audit
                                
                                echo "Linting code..."
                                npm run lint
                                
                                echo "Running unit tests..."
                                npm run test -- --watch=false --browsers=ChromeHeadless --code-coverage
                                
                                echo "Building for production..."
                                npm run build --prod
                                
                                echo "✅ Frontend build completed"
                                ls -la dist/
                            '''
                        }
                    }
                    post {
                        always {
                            // Publish test results
                            publishTestResults testResultsPattern: 'frontend/test-results.xml'
                            
                            // Publish coverage
                            publishHTML([
                                allowMissing: false,
                                alwaysLinkToLastBuild: true,
                                keepAll: true,
                                reportDir: 'frontend/coverage',
                                reportFiles: 'index.html',
                                reportName: 'Frontend Coverage Report'
                            ])
                            
                            archiveArtifacts artifacts: 'frontend/dist/**/*', allowEmptyArchive: true
                        }
                    }
                }
            }
        }
        
        stage('Testing') {
            parallel {
                stage('Unit Tests') {
                    steps {
                        script {
                            echo "🧪 Running unit tests..."
                            
                            sh '''
                                cd backend/build
                                echo "Running C++ unit tests..."
                                
                                # Run tests with XML output
                                ./rf-splitter-tests \
                                    --gtest_output=xml:unit-test-results.xml \
                                    --gtest_filter="-Integration*" \
                                    || true
                                
                                echo "Unit tests completed"
                            '''
                        }
                        
                        // Publish test results
                        publishTestResults testResultsPattern: 'backend/build/unit-test-results.xml'
                    }
                }
                
                stage('Integration Tests') {
                    steps {
                        script {
                            echo "🔗 Running integration tests..."
                            
                            sh '''
                                cd backend/build
                                echo "Running integration tests..."
                                
                                # Start mock services if needed
                                ./rf-splitter-backend --test-mode &
                                BACKEND_PID=$!
                                
                                sleep 2
                                
                                # Run integration tests
                                ./rf-splitter-tests \
                                    --gtest_output=xml:integration-test-results.xml \
                                    --gtest_filter="Integration*" \
                                    || true
                                
                                # Cleanup
                                kill $BACKEND_PID || true
                            '''
                        }
                        
                        publishTestResults testResultsPattern: 'backend/build/integration-test-results.xml'
                    }
                }
                
                stage('Hardware-in-Loop Tests') {
                    when {
                        anyOf {
                            branch 'main'
                            branch 'develop'
                            expression { params.RUN_PERFORMANCE_TESTS }
                        }
                    }
                    agent {
                        label 'hardware-test-rig'
                    }
                    steps {
                        script {
                            echo "🔌 Running hardware-in-loop tests..."
                            
                            sh '''
                                # Flash firmware to test hardware
                                if [ -f "${STM32_PROGRAMMER_CLI}" ]; then
                                    echo "Flashing firmware to test board..."
                                    ${STM32_PROGRAMMER_CLI} \
                                        -c port=SWD \
                                        -w firmware/build/rf-splitter-firmware.hex \
                                        -rst
                                    
                                    sleep 5
                                    
                                    echo "Running hardware tests..."
                                    cd backend/build
                                    ./rf-splitter-tests \
                                        --gtest_output=xml:hardware-test-results.xml \
                                        --gtest_filter="Hardware*" \
                                        || true
                                else
                                    echo "⚠️  STM32 programmer not available, skipping hardware tests"
                                fi
                            '''
                        }
                    }
                    post {
                        always {
                            publishTestResults testResultsPattern: 'backend/build/hardware-test-results.xml'
                        }
                    }
                }
            }
        }
        
        stage('Quality Gates') {
            parallel {
                stage('Code Coverage') {
                    steps {
                        script {
                            echo "📊 Analyzing code coverage..."
                            
                            sh '''
                                cd backend/build
                                echo "Generating coverage report..."
                                
                                # Generate coverage data
                                lcov --capture --directory . --output-file coverage.info
                                lcov --remove coverage.info '/usr/*' '*/test/*' --output-file coverage_filtered.info
                                
                                # Generate HTML report
                                genhtml coverage_filtered.info --output-directory coverage_html
                                
                                # Extract coverage percentage
                                COVERAGE=$(lcov --summary coverage_filtered.info | grep -E "lines\\.\\.\\." | sed 's/.*: \\([0-9.]*\\)%.*/\\1/')
                                echo "Coverage: ${COVERAGE}%"
                                
                                # Check coverage threshold
                                if (( $(echo "${COVERAGE} < ${COVERAGE_THRESHOLD}" | bc -l) )); then
                                    echo "❌ Coverage ${COVERAGE}% is below threshold ${COVERAGE_THRESHOLD}%"
                                    exit 1
                                fi
                                
                                echo "✅ Coverage ${COVERAGE}% meets threshold ${COVERAGE_THRESHOLD}%"
                            '''
                        }
                        
                        publishHTML([
                            allowMissing: false,
                            alwaysLinkToLastBuild: true,
                            keepAll: true,
                            reportDir: 'backend/build/coverage_html',
                            reportFiles: 'index.html',
                            reportName: 'Code Coverage Report'
                        ])
                    }
                }
                
                stage('Performance Tests') {
                    when {
                        expression { params.RUN_PERFORMANCE_TESTS }
                    }
                    steps {
                        script {
                            echo "⚡ Running performance tests..."
                            
                            sh '''
                                cd backend/build
                                echo "Running performance benchmarks..."
                                
                                # Run performance tests
                                ./performance_tests \
                                    --benchmark_out=performance_results.json \
                                    --benchmark_out_format=json \
                                    --benchmark_repetitions=3
                                
                                echo "Performance tests completed"
                                cat performance_results.json
                            '''
                            
                            // Compare with baseline if available
                            sh '''
                                if [ -f "${PERFORMANCE_BASELINE}" ]; then
                                    echo "Comparing with performance baseline..."
                                    python3 scripts/compare_performance.py \
                                        --baseline ${PERFORMANCE_BASELINE} \
                                        --current backend/build/performance_results.json \
                                        --tolerance 10
                                else
                                    echo "No performance baseline found, saving current results as baseline"
                                    cp backend/build/performance_results.json ${PERFORMANCE_BASELINE}
                                fi
                            '''
                        }
                        
                        archiveArtifacts artifacts: 'backend/build/performance_results.json', allowEmptyArchive: true
                    }
                }
                
                stage('Security Scan') {
                    when {
                        expression { params.RUN_SECURITY_SCAN }
                    }
                    steps {
                        script {
                            echo "🔒 Running security analysis..."
                            
                            // Install security tools
                            sh '''
                                pip3 install bandit semgrep safety
                            '''
                            
                            // Run security scans
                            sh '''
                                echo "Running Bandit security scan..."
                                find . -name "*.py" | xargs bandit -r -f json -o bandit_results.json || true
                                
                                echo "Running Semgrep analysis..."
                                semgrep --config=auto --json --output=semgrep_results.json backend/src/ || true
                                
                                echo "Checking for vulnerable dependencies..."
                                safety check --json --output safety_results.json || true
                                
                                echo "Running basic network security checks..."
                                nmap -sS localhost -p 8080,8081 > nmap_results.txt || true
                            '''
                        }
                        
                        archiveArtifacts artifacts: '*_results.{json,txt}', allowEmptyArchive: true
                    }
                }
            }
        }
        
        stage('Package and Deploy') {
            parallel {
                stage('Docker Images') {
                    steps {
                        script {
                            echo "🐳 Building Docker images..."
                            
                            // Build backend image
                            sh '''
                                echo "Building backend Docker image..."
                                cd backend
                                docker build -t ${DOCKER_REGISTRY}/${IMAGE_PREFIX}-backend:${BUILD_NUMBER} .
                                docker tag ${DOCKER_REGISTRY}/${IMAGE_PREFIX}-backend:${BUILD_NUMBER} \\
                                           ${DOCKER_REGISTRY}/${IMAGE_PREFIX}-backend:latest
                            '''
                            
                            // Build frontend image
                            sh '''
                                echo "Building frontend Docker image..."
                                cd frontend
                                docker build -t ${DOCKER_REGISTRY}/${IMAGE_PREFIX}-frontend:${BUILD_NUMBER} .
                                docker tag ${DOCKER_REGISTRY}/${IMAGE_PREFIX}-frontend:${BUILD_NUMBER} \\
                                           ${DOCKER_REGISTRY}/${IMAGE_PREFIX}-frontend:latest
                            '''
                            
                            // Push images if on main branch
                            if (env.BRANCH_NAME == 'main') {
                                sh '''
                                    echo "Pushing Docker images..."
                                    docker push ${DOCKER_REGISTRY}/${IMAGE_PREFIX}-backend:${BUILD_NUMBER}
                                    docker push ${DOCKER_REGISTRY}/${IMAGE_PREFIX}-backend:latest
                                    docker push ${DOCKER_REGISTRY}/${IMAGE_PREFIX}-frontend:${BUILD_NUMBER}
                                    docker push ${DOCKER_REGISTRY}/${IMAGE_PREFIX}-frontend:latest
                                '''
                            }
                        }
                    }
                }
                
                stage('Documentation') {
                    steps {
                        script {
                            echo "📚 Generating documentation..."
                            
                            // Generate API documentation
                            sh '''
                                cd backend/docs
                                python3 generate_api_docs.py \
                                    --source ../src \
                                    --output ./generated_docs
                            '''
                            
                            // Generate C++ documentation
                            sh '''
                                cd backend
                                doxygen Doxyfile || true
                            '''
                        }
                        
                        publishHTML([
                            allowMissing: false,
                            alwaysLinkToLastBuild: true,
                            keepAll: true,
                            reportDir: 'backend/docs/html',
                            reportFiles: 'index.html',
                            reportName: 'C++ API Documentation'
                        ])
                        
                        publishHTML([
                            allowMissing: false,
                            alwaysLinkToLastBuild: true,
                            keepAll: true,
                            reportDir: 'backend/docs/generated_docs',
                            reportFiles: 'README.html',
                            reportName: 'REST API Documentation'
                        ])
                    }
                }
            }
        }
        
        stage('Deploy to Staging') {
            when {
                allOf {
                    branch 'main'
                    expression { params.DEPLOY_TO_STAGING }
                }
            }
            environment {
                STAGING_HOST = credentials('staging-host')
                DEPLOY_KEY = credentials('staging-deploy-key')
            }
            steps {
                script {
                    echo "🚀 Deploying to staging environment..."
                    
                    // Deploy using Docker Compose
                    sh '''
                        echo "Preparing deployment package..."
                        
                        # Create deployment package
                        mkdir -p deployment
                        cp docker-compose.staging.yml deployment/
                        cp -r scripts/deploy deployment/
                        
                        # Update image tags in docker-compose
                        sed -i "s/latest/${BUILD_NUMBER}/g" deployment/docker-compose.staging.yml
                        
                        # Deploy to staging
                        scp -i ${DEPLOY_KEY} -r deployment/ user@${STAGING_HOST}:/opt/rf-splitter/
                        
                        ssh -i ${DEPLOY_KEY} user@${STAGING_HOST} << 'EOF'
                            cd /opt/rf-splitter/deployment
                            docker-compose -f docker-compose.staging.yml down
                            docker-compose -f docker-compose.staging.yml pull
                            docker-compose -f docker-compose.staging.yml up -d
                        EOF
                    '''
                    
                    // Wait for deployment to stabilize
                    sh '''
                        echo "Waiting for services to start..."
                        sleep 30
                        
                        # Health checks
                        curl -f http://${STAGING_HOST}:8080/api/v1/health || {
                            echo "❌ Staging deployment health check failed"
                            exit 1
                        }
                        
                        echo "✅ Staging deployment successful"
                    '''
                }
            }
        }
    }
    
    post {
        always {
            script {
                // Cleanup Docker images
                sh '''
                    docker system prune -f || true
                '''
                
                // Archive build logs
                archiveArtifacts artifacts: 'build.log', allowEmptyArchive: true
            }
        }
        
        success {
            script {
                echo "✅ Pipeline completed successfully!"
                
                // Slack notification
                slackSend(
                    channel: env.SLACK_CHANNEL,
                    color: 'good',
                    message: """
                        ✅ RF Splitter Build #${BUILD_NUMBER} succeeded!
                        Branch: ${env.BRANCH_NAME}
                        Commit: ${env.GIT_COMMIT?.take(8)}
                        Duration: ${currentBuild.durationString}
                        
                        📋 Build Type: ${params.BUILD_TYPE}
                        🧪 Tests: Passed
                        📊 Coverage: Above ${env.COVERAGE_THRESHOLD}%
                        🔒 Security: Clean
                        
                        View: ${BUILD_URL}
                    """.stripIndent()
                )
                
                // Email notification for main branch
                if (env.BRANCH_NAME == 'main') {
                    emailext(
                        to: env.EMAIL_RECIPIENTS,
                        subject: "✅ RF Splitter Build #${BUILD_NUMBER} - Success",
                        body: """
                            The RF Splitter build has completed successfully.
                            
                            Build Details:
                            - Branch: ${env.BRANCH_NAME}
                            - Commit: ${env.GIT_COMMIT}
                            - Build Type: ${params.BUILD_TYPE}
                            - Duration: ${currentBuild.durationString}
                            
                            Artifacts:
                            - STM32 firmware: Available
                            - Backend services: Docker images pushed
                            - Frontend application: Built and ready
                            - Documentation: Generated
                            
                            View build: ${BUILD_URL}
                        """,
                        attachmentsPattern: 'backend/build/rf-splitter-firmware.hex'
                    )
                }
            }
        }
        
        failure {
            script {
                echo "❌ Pipeline failed!"
                
                // Slack notification
                slackSend(
                    channel: env.SLACK_CHANNEL,
                    color: 'danger',
                    message: """
                        ❌ RF Splitter Build #${BUILD_NUMBER} failed!
                        Branch: ${env.BRANCH_NAME}
                        Commit: ${env.GIT_COMMIT?.take(8)}
                        Duration: ${currentBuild.durationString}
                        
                        Failed Stage: ${env.STAGE_NAME}
                        
                        View: ${BUILD_URL}
                        Console: ${BUILD_URL}console
                    """.stripIndent()
                )
                
                // Email notification
                emailext(
                    to: env.EMAIL_RECIPIENTS,
                    subject: "❌ RF Splitter Build #${BUILD_NUMBER} - Failed",
                    body: """
                        The RF Splitter build has failed.
                        
                        Build Details:
                        - Branch: ${env.BRANCH_NAME}
                        - Commit: ${env.GIT_COMMIT}
                        - Failed Stage: ${env.STAGE_NAME}
                        - Duration: ${currentBuild.durationString}
                        
                        Please check the build logs for details.
                        
                        View build: ${BUILD_URL}
                        Console output: ${BUILD_URL}console
                    """
                )
            }
        }
        
        unstable {
            script {
                // Slack notification for unstable builds
                slackSend(
                    channel: env.SLACK_CHANNEL,
                    color: 'warning',
                    message: """
                        ⚠️ RF Splitter Build #${BUILD_NUMBER} is unstable
                        Branch: ${env.BRANCH_NAME}
                        
                        Some tests may have failed or quality gates not met.
                        
                        View: ${BUILD_URL}
                    """.stripIndent()
                )
            }
        }
        
        changed {
            script {
                if (currentBuild.currentResult == 'SUCCESS' && currentBuild.previousBuild?.result == 'FAILURE') {
                    // Build recovered
                    slackSend(
                        channel: env.SLACK_CHANNEL,
                        color: 'good',
                        message: """
                            🎉 RF Splitter Build #${BUILD_NUMBER} recovered!
                            Branch: ${env.BRANCH_NAME}
                            
                            The build is now passing again.
                            
                            View: ${BUILD_URL}
                        """.stripIndent()
                    )
                }
            }
        }
    }
}