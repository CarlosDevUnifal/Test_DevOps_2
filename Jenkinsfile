pipeline {
    agent { label 'cpp' }
    options { buildDiscarder(logRotator(numToKeepStr: '10')); timestamps(); timeout(time: 30, unit: 'MINUTES') }
    triggers { cron('0 8 * * *') }

    environment {
        PROJECT_DIR = 'calculator'
    }

    stages {
        stage('Checkout') {
            steps {
                echo '=== Obtendo código fonte ==='
                checkout scm
                sh 'ls -la'
                sh 'ls -la calculator'
            }
        }

        stage('Code Quality') {
            steps {
                dir(env.PROJECT_DIR) {
                    sh 'make check'
                }
            }
        }

        stage('Build') {
            steps {
                dir(env.PROJECT_DIR) {
                    sh 'make clean || true'
                    sh 'make'
                    sh 'ls -la bin/'
                }
            }
        }

        stage('Test') {
            steps {
                dir(env.PROJECT_DIR) {
                    sh 'make unittest'
                }
            }
        }

        stage('Archive Artifacts') {
            steps {
                dir(env.PROJECT_DIR) {
                    archiveArtifacts artifacts: 'bin/calculator', fingerprint: true
                }
            }
        }
    }

    post {
        success { echo '✅ Pipeline executado com sucesso!' }
        failure { echo '❌ Pipeline falhou! Verifique os logs.' }
        always { echo "Build #${BUILD_NUMBER} finalizado"; cleanWs() }
    }
}
