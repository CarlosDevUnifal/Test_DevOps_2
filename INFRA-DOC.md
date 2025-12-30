# Documentação da Infraestrutura CI/CD

## Visão Geral

Implementação de pipeline de integração e entrega contínua para projeto C++17 utilizando Jenkins e Docker, conforme especificação do exercício.

---

## 1. Infraestrutura

### 1.1 Servidor Host
| Item | Especificação |
|------|---------------|   
| Sistema Operacional | Ubuntu 24.04 LTS |
| Docker Engine | Instalado via repositório oficial |
| Swap | 2 GB (para evitar OOM em máquinas com RAM limitada) |

### 1.2 Containers Docker

#### Jenkins Controller
```bash
docker run -d \
  --name jenkins \
  -p 8080:8080 -p 50000:50000 \
  -v jenkins_home:/var/jenkins_home \
  --restart unless-stopped \
  jenkins/jenkins:lts
```

#### Agentes de Build (2 nós)

**Dockerfile do Agente (`Dockerfile.agent`):**
```dockerfile
FROM jenkins/inbound-agent:latest

USER root

# Ferramentas de build C++17
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    clang-tidy \
    clang-format \
    cmake \
    libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

# Compilar e instalar Google Test
RUN cd /usr/src/googletest/googletest \
    && cmake -B build \
    && cmake --build build \
    && cp build/lib/*.a /usr/local/lib/

USER jenkins
```

**Build da imagem:**
```bash
docker build -t jenkins-agent-cpp:latest -f Dockerfile.agent .
```

**Execução dos agentes:**
```bash
# Agente 1
docker run -d \
  --name cpp-agent-1 \
  --init \
  jenkins-agent-cpp:latest \
  -url http://<JENKINS_IP>:8080 \
  -secret <SECRET_AGENT_1> \
  -name cpp-agent-1 \
  -workDir /home/jenkins/agent

# Agente 2
docker run -d \
  --name cpp-agent-2 \
  --init \
  jenkins-agent-cpp:latest \
  -url http://<JENKINS_IP>:8080 \
  -secret <SECRET_AGENT_2> \
  -name cpp-agent-2 \
  -workDir /home/jenkins/agent
```

### 1.3 Configuração dos Nós no Jenkins

1. Acesse **Manage Jenkins → Nodes → New Node**
2. Configure cada agente:
   - **Name:** `cpp-agent-1` / `cpp-agent-2`
   - **Type:** Permanent Agent
   - **Remote root directory:** `/home/jenkins/agent`
   - **Labels:** `cpp`
   - **Launch method:** Launch agent by connecting it to the controller

---

## 2. Pipeline CI/CD

### 2.1 Estrutura do Jenkinsfile (atual)

```groovy
pipeline {
    agent none
    options {
        buildDiscarder(logRotator(numToKeepStr: '10'))
        timestamps()
        timeout(time: 30, unit: 'MINUTES')
    }
    triggers {
        cron('0 8 * * *')
    }

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
            script { node('cpp') { cleanWs() } }
        }
    }
}
```

### 2.2 Stages do Pipeline

| Stage | Comando | Descrição |
|-------|---------|-----------|
| **Checkout** | `checkout scm` | Obtém código fonte do repositório Git |
| **Code Quality (matrix)** | `make check` em `cpp-agent-1` e `cpp-agent-2` (unstash + ws isolado) | Lint/format em ambos os nós |
| **Build & Test** | `make clean || true`; `make`; `make unittest` | Compila e executa testes (artefato único) |
| **Archive Artifacts** | `archiveArtifacts bin/calculator` | Armazena binário e fingerprinta |

### 2.3 Triggers Configurados

| Tipo | Configuração | Descrição |
|------|--------------|-----------|
| **Manual** | Botão "Build Now" | Execução sob demanda |
| **Agendado** | `cron('0 8 * * *')` | Execução diária às 08:00 UTC |
| **Webhook** | (Opcional) | Integração com GitHub para builds automáticos em push |

---

## 3. Estrutura do Projeto

```
Test_DevOps_2/
├── Jenkinsfile              # Pipeline declarativo
├── README.md
├── .gitignore
└── calculator/
    ├── Makefile             # Alvos: all, check, unittest, clean
    ├── README.md
    ├── src/
    │   ├── main.cpp         # Aplicação principal
    │   └── calculator.hpp   # Classe Calculator<T>
    └── tests/
        ├── Makefile
        ├── main.cpp
        └── test_Calculator.cpp  # Testes GTest
```

### 3.1 Comandos do Makefile

| Comando | Descrição |
|---------|-----------|
| `make` | Compila o projeto, gera `bin/calculator` |
| `make check` | Executa `lint` (clang-tidy) e `format-check` (clang-format) |
| `make unittest` | Compila e executa testes unitários |
| `make clean` | Remove artefatos de build |

---

## 4. Tratamento de Erros

O pipeline considera **qualquer falha como crítica**:

- **clang-tidy:** `warnings-as-errors='*'` — warnings viram erros
- **clang-format:** `-dry-run` com `-Wclang-format-violations` — código não formatado falha
- **Build:** Erros de compilação interrompem o pipeline
- **Testes:** Falhas nos testes GTest resultam em exit code != 0

---

## 5. Validação Final

### 5.1 Checklist de Requisitos

| Requisito | Status |
|-----------|--------|
| Jenkins com Docker | ✅ |
| Mínimo 2 agentes de execução | ✅ (cpp-agent-1, cpp-agent-2) |
| Ambiente C++17 nos agentes | ✅ |
| Gatilho manual | ✅ |
| Agendamento diário | ✅ (cron 0 8 * * *) |
| Obtenção do código fonte | ✅ (stage Checkout) |
| Checagem de código (lint/format) | ✅ (stage Code Quality) |
| Execução de testes | ✅ (stage Test) |
| Geração de artefatos | ✅ (stage Build) |
| Armazenamento de artefatos | ✅ (stage Archive Artifacts) |
| Falhas tratadas como críticas | ✅ |

### 5.2 Evidência de Execução

```
Started by user Carlos Henrique Arantes
Running on cpp-agent-1
Commit: "resolvendo warning [-Wclang-format-violations]"

Stages:
  ✅ Checkout
  ✅ Code Quality (clang-tidy + clang-format)
  ✅ Build (g++ -std=c++17)
  ✅ Test (2 tests PASSED)
  ✅ Archive Artifacts (bin/calculator)

Finished: SUCCESS
```

---

## 6. Referências

- **Repositório:** https://github.com/CarlosDevUnifal/Test_DevOps_2
- **Jenkins LTS:** 2.528.3
- **Docker Image Agente:** jenkins/inbound-agent:latest (customizado)
- **Compilador:** g++ 13.x (Ubuntu 24.04)
- **Framework de Testes:** Google Test 1.14.0

---

## 7. Comandos Úteis

```bash
# Verificar containers em execução
docker ps

# Logs do Jenkins
docker logs jenkins

# Logs dos agentes
docker logs cpp-agent-1
docker logs cpp-agent-2

# Reiniciar Jenkins
docker restart jenkins

# Executar make check local (no servidor)
cd ~/jenkins-devops/Test_DevOps_2/calculator && make check && make && make unittest
```

---

**Autor:** Carlos Henrique Arantes  
**Data:** 19 de Dezembro de 2025
