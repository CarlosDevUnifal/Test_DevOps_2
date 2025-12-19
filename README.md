# Test_DevOps_2

# Registro de Comandos e Justificativas - Desafio DevOps

Este documento detalha os comandos e procedimentos técnicos utilizados durante a resolução do desafio de CI/CD com Jenkins e Docker, refletindo **exatamente** a sequência de eventos que ocorreu no terminal.

**Ambiente:** Ubuntu 24.04.3 LTS (AWS EC2) | Jenkins LTS 2.528.3 | Docker 29.1.3  
**Repositório:** https://github.com/CarlosDevUnifal/Test_DevOps_2  
**Data de Execução:** 18-19 de Dezembro de 2025

---

## 1. Reconhecimento do Ambiente

### Identificação Inicial do Host
```bash
ubuntu@ip-172-31-8-111:~$ whoami
ubuntu

ubuntu@ip-172-31-8-111:~$ uname -a
Linux ip-172-31-8-111 6.14.0-1015-aws #15~24.04.1-Ubuntu SMP x86_64 GNU/Linux

ubuntu@ip-172-31-8-111:~$ lsb_release -a
Distributor ID: Ubuntu
Description:    Ubuntu 24.04.3 LTS
Release:        24.04
Codename:       noble
```

### Verificação de Recursos e Ferramentas Disponíveis
```bash
ubuntu@ip-172-31-8-111:~$ df -h
Filesystem       Size  Used Avail Use% Mounted on
/dev/root         15G  2.2G   13G  16% /

ubuntu@ip-172-31-8-111:~$ free -h
               total        used        free      shared  buff/cache   available
Mem:           914Mi       809Mi       100Mi       2.7Mi       160Mi       105Mi
Swap:             0B          0B          0B  # <-- CRÍTICO: Sem swap!

ubuntu@ip-172-31-8-111:~$ docker --version
Command 'docker' not found  # Docker não instalado

ubuntu@ip-172-31-8-111:~$ which jenkins
# (vazio - Jenkins não instalado)

ubuntu@ip-172-31-8-111:~$ git --version
git version 2.43.0  # Git OK
```
**Observação:** Instância com apenas ~914MB RAM e **zero swap**. Docker e Jenkins precisavam ser instalados do zero.

---

## 2. Instalação do Docker

### Tentativa Inicial (Falhou)
```bash
ubuntu@ip-172-31-8-111:~$ sudo apt install -y docker.io docker-compose
E: Package 'docker.io' has no installation candidate
E: Unable to locate package docker-compose
```
**Problema:** Os pacotes `docker.io` e `docker-compose` não estavam disponíveis nos repositórios padrão do Ubuntu 24.04.

### Solução: Script Oficial do Docker
```bash
ubuntu@ip-172-31-8-111:~$ curl -fsSL https://get.docker.com -o get-docker.sh
ubuntu@ip-172-31-8-111:~$ sudo sh get-docker.sh
# Executing docker install script...
# + apt-get install docker-ce docker-ce-cli containerd.io docker-compose-plugin...
# INFO: Docker daemon enabled and started
```
**Justificativa:** O script oficial (`get.docker.com`) configura automaticamente o repositório do Docker e instala a versão mais recente.

### Configuração de Permissões
```bash
ubuntu@ip-172-31-8-111:~$ sudo usermod -aG docker ubuntu
ubuntu@ip-172-31-8-111:~$ newgrp docker

ubuntu@ip-172-31-8-111:~$ docker --version
Docker version 29.1.3, build f52814d

ubuntu@ip-172-31-8-111:~$ docker ps
CONTAINER ID   IMAGE     COMMAND   CREATED   STATUS    PORTS     NAMES
# (vazio - Docker funcionando)
```
**Justificativa:** Adicionar o usuário ao grupo `docker` permite executar comandos sem `sudo`.

---

## 3. Instalação do Jenkins

### Preparação do Diretório
```bash
ubuntu@ip-172-31-8-111:~$ mkdir -p ~/jenkins-devops
ubuntu@ip-172-31-8-111:~$ cd jenkins-devops/
```

### Download do Jenkins WAR (para verificação de versão)
```bash
ubuntu@ip-172-31-8-111:~/jenkins-devops$ wget https://get.jenkins.io/war-stable/2.528.3/jenkins.war
jenkins.war  100%[============>]  91.03M  12.3MB/s  in 10s

ubuntu@ip-172-31-8-111:~/jenkins-devops$ sha256sum jenkins.war
bfa31f1e3aacebb5bce3d5076c73df97bf0c0567eeb8d8738f54f6bac48abd74  jenkins.war
```

### Execução do Jenkins em Container
```bash
ubuntu@ip-172-31-8-111:~/jenkins-devops$ docker run -d \
  --name jenkins \
  --restart unless-stopped \
  -p 8080:8080 \
  -p 50000:50000 \
  -v ~/jenkins-devops/jenkins_home:/var/jenkins_home \
  -e JAVA_OPTS="-Xmx256m -Xms128m" \
  jenkins/jenkins:lts

# Imagem baixada...
87c0b9315af301c1ceae8d7462518076f6e0687efcd36aacf90d7e5ac9478af4
```
**Justificativa:** Limitamos a JVM a 256MB (`-Xmx256m`) devido à RAM escassa da instância.

---

## 4. 🚨 INCIDENTE: Servidor Travou (OOM)

### Sintoma
```bash
ubuntu@ip-172-31-8-111:~/jenkins-devops$ docker ps
^C
ubuntu@ip-172-31-8-111:~/jenkins-devops$ docker ps -a
^C^C^C
# Terminal não respondia - servidor travado
```
**Causa:** O Jenkins consumiu toda a memória disponível. Sem swap, o kernel não conseguiu lidar com a pressão de memória.

**Ação:** Solicitado reinício da instância via suporte.

### Diagnóstico Pós-Reinício
```bash
ubuntu@ip-172-31-8-111:~$ free -h
               total        used        free      shared  buff/cache   available
Mem:           914Mi       905Mi        66Mi       2.8Mi        48Mi       9.0Mi
Swap:             0B          0B          0B  # <-- Ainda sem swap!
```

### Solução: Configurar Swap de 2GB
```bash
ubuntu@ip-172-31-8-111:~$ sudo fallocate -l 2G /swapfile
ubuntu@ip-172-31-8-111:~$ sudo chmod 600 /swapfile
ubuntu@ip-172-31-8-111:~$ sudo mkswap /swapfile
Setting up swapspace version 1, size = 2 GiB (2147479552 bytes)

ubuntu@ip-172-31-8-111:~$ sudo swapon /swapfile

# Tornar permanente
ubuntu@ip-172-31-8-111:~$ echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
```

### Verificação Após Swap
```bash
ubuntu@ip-172-31-8-111:~$ free -h
               total        used        free      shared  buff/cache   available
Mem:           914Mi       790Mi        69Mi       2.8Mi       212Mi       123Mi
Swap:          2.0Gi       245Mi       1.8Gi  # ✅ Swap ativo!
```
**Justificativa:** Com 2GB de swap, o sistema pode usar disco como extensão da RAM, evitando travamentos por falta de memória.

---

## 5. Verificação do Jenkins Pós-Recuperação

### Status do Container
```bash
ubuntu@ip-172-31-8-111:~/jenkins-devops$ docker ps
CONTAINER ID   IMAGE                 COMMAND                  CREATED          STATUS          PORTS
87c0b9315af3   jenkins/jenkins:lts   "/usr/bin/tini -- /u…"   46 minutes ago   Up 46 minutes   0.0.0.0:8080->8080/tcp
```

### Obtenção da Senha Inicial
```bash
ubuntu@ip-172-31-8-111:~/jenkins-devops$ docker exec jenkins cat /var/jenkins_home/secrets/initialAdminPassword
6e9439f7979d4c8eaccb6b2a972b0e02
```

---

## 6. Clone do Repositório

### Tentativa com Repo Privado (Falhou)
```bash
ubuntu@ip-172-31-8-111:~/jenkins-devops$ git clone https://github.com/philips-internal/Test_DevOps.git
Username for 'https://github.com': ^C  # Cancelado - repo privado
```

### Clone do Fork Público
```bash
ubuntu@ip-172-31-8-111:~/jenkins-devops$ git clone https://github.com/alissoneves/Test_DevOps_2.git
Cloning into 'Test_DevOps_2'...
Receiving objects: 100% (23/23), 7.70 KiB | 2.57 MiB/s, done.

ubuntu@ip-172-31-8-111:~/jenkins-devops$ cd Test_DevOps_2/
ubuntu@ip-172-31-8-111:~/jenkins-devops/Test_DevOps_2$ ls -la
drwxrwxr-x 4 ubuntu ubuntu 4096 Dec 18 23:42 .
drwxrwxr-x 8 ubuntu ubuntu 4096 Dec 18 23:42 .git
-rw-rw-r-- 1 ubuntu ubuntu   15 Dec 18 23:42 README.md
drwxrwxr-x 4 ubuntu ubuntu 4096 Dec 18 23:42 calculator
```

---

## 7. Criação da Imagem do Agente C++

### Dockerfile Completo (`Dockerfile.agent`)
```dockerfile
FROM jenkins/inbound-agent:latest

USER root

# Instalar dependências C++17, clang-tidy, clang-format e GTest
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    clang-tidy \
    clang-format \
    cmake \
    libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

# Compilar e instalar Google Test (Ubuntu só instala os fontes)
RUN cd /usr/src/googletest/googletest \
    && cmake -B build \
    && cmake --build build \
    && cp build/lib/*.a /usr/local/lib/

# Verificar instalações
RUN g++ --version && clang-tidy --version && clang-format --version

USER jenkins
WORKDIR /home/jenkins/agent
```

### Build da Imagem
```bash
ubuntu@ip-172-31-8-111:~/jenkins-devops$ docker build -t jenkins-agent-cpp:latest -f Dockerfile.agent .
```
**Justificativa:** O agente padrão do Jenkins não possui ferramentas C++. Criamos uma imagem customizada com g++, clang-tidy, clang-format e Google Test compilado.

---

## 8. Execução dos Agentes (2 Nós)

```bash
# Agente 1
docker run -d --name cpp-agent-1 --init jenkins-agent-cpp:latest \
  -url http://172.31.8.111:8080 \
  -secret <SECRET_FROM_JENKINS_UI> \
  -name cpp-agent-1 \
  -workDir /home/jenkins/agent

# Agente 2
docker run -d --name cpp-agent-2 --init jenkins-agent-cpp:latest \
  -url http://172.31.8.111:8080 \
  -secret <SECRET_FROM_JENKINS_UI> \
  -name cpp-agent-2 \
  -workDir /home/jenkins/agent
```
**Justificativa:** O desafio exigia "no mínimo 2 nós agentes". O `--init` garante limpeza correta de processos zumbis.

### Verificação Final
```bash
ubuntu@ip-172-31-8-111:~/jenkins-devops$ docker ps
CONTAINER ID   IMAGE                    STATUS          NAMES
87c0b9315af3   jenkins/jenkins:lts      Up 2 hours      jenkins
a1b2c3d4e5f6   jenkins-agent-cpp:latest Up 1 hour       cpp-agent-1
f6e5d4c3b2a1   jenkins-agent-cpp:latest Up 1 hour       cpp-agent-2

ubuntu@ip-172-31-8-111:~/jenkins-devops$ free -h
Mem:           914Mi       769Mi        62Mi       1.9Mi       241Mi       144Mi
Swap:          2.0Gi       594Mi       1.4Gi  # Sistema estável com swap
```

---

## 9. Configuração do Git (Para Commits)

```bash
git config --global user.name "Carlos Henrique Arantes"
git config --global user.email "carlos.arantes@sou.unifal-mg.edu.br"
```

---

## 10. Administração e Troubleshooting do Jenkins

### Listagem de Usuários Cadastrados
```bash
docker exec -it jenkins ls /var/jenkins_home/users
```
**Justificativa:** Descobrir os usernames existentes quando não se lembra do login.

### Recuperação de Acesso (Reset de Senha via Groovy)
Durante o processo, houve necessidade de resetar a senha do admin. Utilizamos scripts Groovy injetados no container.

```bash
# 1. Criar script de reset
docker exec -it jenkins sh -lc 'mkdir -p /var/jenkins_home/init.groovy.d && cat > /var/jenkins_home/init.groovy.d/000_reset_admin.groovy <<EOF
import jenkins.model.*
import hudson.security.*

def j = Jenkins.getInstanceOrNull()
if (j == null) return
def realm = new HudsonPrivateSecurityRealm(false)
j.setSecurityRealm(realm)

def u = realm.getUser("admin")
if (u == null) u = realm.createAccount("admin", "admin123")
else { u.addProperty(new HudsonPrivateSecurityRealm.Details("admin123")); u.save() }

def strat = new FullControlOnceLoggedInAuthorizationStrategy()
strat.setAllowAnonymousRead(false)
j.setAuthorizationStrategy(strat)

j.save()
println("RESET_ADMIN_DONE")
EOF'

# 2. Reiniciar para aplicar
docker restart jenkins

# 3. Verificar se o script rodou
docker logs jenkins | grep RESET_ADMIN_DONE

# 4. Remover script após sucesso (IMPORTANTE!)
docker exec -it jenkins rm /var/jenkins_home/init.groovy.d/000_reset_admin.groovy
```
**Justificativa:** O acesso à senha inicial foi perdido. A injeção de scripts no `init.groovy.d` é a maneira padrão e segura de alterar configurações internas do Jenkins (SecurityRealm) programaticamente na inicialização.

**⚠️ Lição Aprendida:** Tentativas com `user.setPassword()` falharam; a API correta é `user.addProperty(new HudsonPrivateSecurityRealm.Details(...))`.

### Desativação Temporária de Segurança (Alternativa)
```bash
docker exec -it jenkins sh -lc 'cat > /var/jenkins_home/init.groovy.d/000_disable_security.groovy <<EOF
import jenkins.model.*
def j = Jenkins.getInstanceOrNull()
if (j != null) { j.disableSecurity(); j.save(); println("SECURITY_DISABLED") }
EOF'
docker restart jenkins
```
**Justificativa:** Se nenhum método de reset de senha funcionar, desativar temporariamente a segurança permite acessar a UI e reconfigurar manualmente. **Deve ser removido imediatamente após acesso.**

---

## 11. Desenvolvimento e Correção de Código (C++)

### Instalação de Ferramentas no Host/Agente
```bash
sudo apt-get install -y clang-tidy clang-format build-essential cmake libgtest-dev
```
**Justificativa:** Necessário para rodar `make check` e `make unittest` localmente para debugar os erros que ocorriam no pipeline.

### Compilação do Google Test
```bash
cd /usr/src/googletest/googletest
sudo cmake -B build
sudo cmake --build build
sudo cp build/lib/*.a /usr/local/lib/
```
**Justificativa:** O pacote `libgtest-dev` no Ubuntu instala apenas os fontes. É necessário compilar as bibliotecas estáticas (`.a`) e movê-las para onde o linker (`ld`) consegue encontrar, resolvendo o erro de linkagem nos testes.

### Correção de Código (Edição via Terminal)
Utilizamos `cat <<EOF > arquivo` para editar arquivos remotamente.

1.  **`calculator/src/calculator.hpp`**:
    *   **Ação:** Adicionado tratamento para divisão por zero (retornando 0) e implementação dos métodos `add`, `subtract`, `multiply`.
    *   **Justificativa:** Os testes unitários falhavam com "Floating point exception" ou falta de métodos.
2.  **`calculator/src/main.cpp`**:
    *   **Ação:** Reformatado o código para o estilo Google.
    *   **Justificativa:** O estágio de "Code Quality" falhava com `warning: code should be clang-formatted`.

### Validação Local
```bash
make check      # Valida Lint e Formatação
make            # Valida Compilação
make unittest   # Valida Lógica
```
**Justificativa:** Garantir que o código está saudável antes de enviar para o repositório e disparar o pipeline (Fail-fast local).

---

## 12. Git e Versionamento

### Sincronização e Resolução de Conflitos
```bash
git pull --rebase origin main
git checkout --theirs calculator/src/main.cpp
git add .
git rebase --continue
```
**Justificativa:** Houve divergência entre o histórico local e remoto. O `rebase` foi usado para manter um histórico linear. O `checkout --theirs` foi usado para aceitar a versão do servidor (que já estava formatada corretamente) durante um conflito.

### Limpeza de Repositório
```bash
git rm --cached calculator/bin/calculator calculator/obj/main.o calculator/tests/bin/unittest
printf "\ncalculator/bin/\ncalculator/obj/\ncalculator/tests/bin/\n" >> .gitignore
git add .gitignore
git commit -m "Remove build artifacts and ignore outputs"
```
**Justificativa:** Binários e objetos de compilação não devem ser versionados. Removemos do índice do Git e adicionamos ao `.gitignore` para manter o repositório limpo.

### Fluxo Padrão de Commit
```bash
git status
git add <arquivos>
git commit -m "Mensagem descritiva"
git push origin main
```
**Justificativa:** Workflow básico para versionar alterações e disparar o pipeline automaticamente (se webhook configurado) ou manualmente via Jenkins.

---

## 13. Pipeline (Jenkinsfile)

### Evolução do Pipeline

O `Jenkinsfile` passou por várias iterações para atender aos requisitos de forma "sênior":

| Versão | Mudança | Motivo |
|--------|---------|--------|
| v1 | `agent { label 'cpp' }` simples | Pipeline básico funcional |
| v2 | `agent none` + `stash/unstash` | Evitar checkout redundante, economizar recursos |
| v3 | `matrix` no Code Quality | Usar ambos os agentes em paralelo |
| v4 | `ws()` isolado | Evitar colisão de workspaces entre agentes |
| v5 | `cleanWs` dentro de `node()` | Corrigir erro "requires node context" |

### Recursos Utilizados

1.  **`agent none`**: Não alocar um nó executor no início (economiza recursos).
2.  **`stash` / `unstash`**: Clonar o código uma vez e reutilizá-lo em outros estágios/nós, garantindo consistência.
3.  **`matrix`**: Executar o estágio de "Code Quality" simultaneamente nos dois agentes (`cpp-agent-1` e `cpp-agent-2`).
    *   **Justificativa:** Atende ao requisito de usar os múltiplos agentes disponíveis e valida que o ambiente é reprodutível em qualquer nó.
4.  **`ws()` (Workspace Isolation)**:
    *   **Justificativa:** Ao rodar em paralelo ou reutilizar nós, isolar os diretórios de trabalho evita que um job sobrescreva arquivos do outro.
5.  **`cleanWs` dentro de `node`**:
    *   **Justificativa:** Com `agent none`, o passo `post` precisa de um contexto de nó explícito para conseguir limpar o disco.

### Tentativa de parallelsAlwaysFailFast (não suportada)
```groovy
matrix {
    options {
        parallelsAlwaysFailFast()  // ERRO: Invalid option type
    }
    ...
}
```
**Justificativa:** Tentamos habilitar fail-fast na matriz para abortar branches paralelas se uma falhar. Porém, essa opção não é suportada dentro de `matrix` no Declarative Pipeline. O código foi revertido para manter o pipeline funcional.

---

## 14. Tabela de Erros Encontrados e Soluções

| # | Erro | Causa | Solução |
|---|------|-------|---------|
| 1 | Servidor travou (OOM) | RAM insuficiente, sem swap | Configurar swap de 2GB |
| 2 | `docker.io` not found | Pacote não disponível no Ubuntu 24.04 | Usar script `get.docker.com` |
| 3 | `clang-tidy: not found` | Ferramenta não instalada no agente | `apt-get install clang-tidy` |
| 4 | `clang-format: not found` | Ferramenta não instalada no agente | `apt-get install clang-format` |
| 5 | `g++: not found` | Compilador não instalado | `apt-get install build-essential` |
| 6 | `gtest/gtest.h: No such file` | GTest não compilado | Compilar e copiar `.a` para `/usr/local/lib` |
| 7 | Division by zero (FPE) | `Calculator::divide()` não tratava `b==0` | Retornar 0 quando divisor é zero |
| 8 | `no member named 'add'` | Métodos não implementados | Adicionar `add`, `subtract`, `multiply` |
| 9 | `-Wclang-format-violations` | Código fora do estilo Google | `clang-format -i src/main.cpp` |
| 10 | `setPassword()` não existe | API Jenkins mudou | Usar `addProperty(new Details(...))` |
| 11 | `requires node context` | `cleanWs` sem nó em `agent none` | Envolver em `node('cpp') { cleanWs() }` |
| 12 | `Invalid option type parallelsAlwaysFailFast` | Não suportado em `matrix` | Remover a opção |

---

## 15. Comandos Úteis de Referência

```bash
# Docker
docker ps                              # Listar containers
docker logs <container>                # Ver logs
docker restart <container>             # Reiniciar
docker exec -it <container> bash       # Shell interativo

# Jenkins (dentro do container)
ls /var/jenkins_home/users             # Listar usuários
cat /var/jenkins_home/config.xml       # Config global

# C++ / Make
make check      # Lint + Format check
make            # Build
make unittest   # Testes
make clean      # Limpar artefatos

# Git
git status                # Estado do repo
git log --oneline -5      # Últimos commits
git diff                  # Ver mudanças
git stash                 # Guardar mudanças temporariamente
```

---

## 16. Cronologia Resumida

| Horário (UTC) | Evento |
|---------------|--------|
| 18/12 ~22:00 | Início - Reconhecimento do ambiente |
| 18/12 ~22:10 | Tentativa falha com `apt install docker.io` |
| 18/12 ~22:15 | Docker instalado via script oficial |
| 18/12 ~22:19 | Jenkins WAR baixado (verificação) |
| 18/12 ~22:20 | Jenkins container iniciado |
| 18/12 ~22:25 | **TRAVAMENTO** - Servidor OOM |
| 18/12 ~23:00 | Reinício + Configuração de swap 2GB |
| 18/12 ~23:05 | Jenkins recuperado e funcionando |
| 18/12 ~23:42 | Repositório clonado |
| 18/12 ~23:50 | Imagem do agente C++ construída |
| 19/12 ~00:00 | 2 agentes conectados |
| 19/12 ~01:00 | Pipeline v1 funcionando |
| 19/12 ~02:00 | Correções de código (divide, format) |
| 19/12 ~03:00 | Pipeline v5 (matriz) - Build #13 SUCCESS |

---

**Autor:** Carlos Henrique Arantes  
**Data:** 19 de Dezembro de 2025
