pipeline {
    agent none
    options { buildDiscarder(logRotator(numToKeepStr: '10')); timestamps(); timeout(time: 30, unit: 'MINUTES') }
    triggers { cron('0 8 * * *') }

    environment {
        PROJECT_DIR = 'calculator'
    }

    stages {
        stage('Checkout') {
            agent { label 'cpp' }
            steps {
                echo '=== Obtendo código fonte ==='
                checkout scm
                sh 'ls -la'
                sh "ls -la ${env.PROJECT_DIR}"
                stash name: 'source', includes: '**'
            }
        }

        stage('Code Quality (matrix agents)') {
            matrix {
                options {
                    parallelsAlwaysFailFast()
                }
                axes {
                    axis {
                        name 'NODE'
                        values 'cpp-agent-1', 'cpp-agent-2'
                    }
                }
                agent { label "${NODE}" }
                stages {
                    stage('Lint & Format') {
                        steps {
                            ws("workspace/${env.JOB_NAME}/${NODE}") {
                                unstash 'source'
                                dir(env.PROJECT_DIR) {
                                    sh 'make check'
                                }
                            }
                        }
                    }
                }
            }
        }

        stage('Build & Test (single artifact)') {
            agent { label 'cpp' }
            steps {
                ws("workspace/${env.JOB_NAME}/build") {
                    unstash 'source'
                    dir(env.PROJECT_DIR) {
                        sh 'make clean || true'
                        sh 'make'
                        sh 'make unittest'
                        sh 'ls -la bin/'
                    }
                }
            }
        }

        stage('Archive Artifacts') {
            agent { label 'cpp' }
            steps {
                ws("workspace/${env.JOB_NAME}/build") {
                    dir(env.PROJECT_DIR) {
                        archiveArtifacts artifacts: 'bin/calculator', fingerprint: true
                    }
                }
            }
        }
    }

    post {
        success { echo '✅ Pipeline executado com sucesso!' }
        failure { echo '❌ Pipeline falhou! Verifique os logs.' }
        always {
            echo "Build #${BUILD_NUMBER} finalizado"
            script {
                node('cpp') {
                    cleanWs()
                }
            }
        }
    }
}
