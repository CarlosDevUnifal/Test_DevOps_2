pipeline {
    agent {
        label 'cpp'
    }
    
    options {
        buildDiscarder(logRotator(numToKeepStr: '10'))
        timestamps()
        timeout(time: 30, unit: 'MINUTES')
    }
    
    triggers {
        cron('0 8 * * *')  // Agendamento diário às 8h
    }
    
    stages {
        stage('Checkout') {
            steps {
                echo '=== Obtendo código fonte ==='
                checkout scm
                sh 'ls -la'
            }
        }
        
        stage('Code Quality') {
            steps {
                echo '=== Verificando qualidade do código ==='
                sh 'make check'
            }
        }
        
        stage('Build') {
            steps {
                echo '=== Compilando aplicação ==='
                sh 'make clean || true'
                sh 'make'
                sh 'ls -la bin/'
            }
        }
        
        stage('Test') {
            steps {
                echo '=== Executando testes unitários ==='
                sh 'make unittest'
            }
        }
        
        stage('Archive Artifacts') {
            steps {
                echo '=== Armazenando artefatos ==='
                archiveArtifacts artifacts: 'bin/calculator', fingerprint: true
            }
        }
    }
    
    post {
        success {
            echo '✅ Pipeline executado com sucesso!'
        }
        failure {
            echo '❌ Pipeline falhou! Verifique os logs.'
        }
        always {
            echo "Build #${BUILD_NUMBER} finalizado"
            cleanWs()
        }
    }
}
