# Shell Linux

Shell interactiva desarrollada en C para Linux como proyecto de Sistemas Operativos. Implementa ejecucion de comandos, control de trabajos, redirecciones, procesos en segundo plano y varias ampliaciones relacionadas con senales e hilos.

> Este proyecto usa llamadas propias de sistemas Unix/Linux como `fork`, `execvp`, `waitpid`, `setpgid`, `tcsetpgrp` y senales POSIX. No esta pensado para ejecutarse directamente como `.exe` nativo de Windows.

## Caracteristicas

- Ejecucion de comandos del sistema.
- Comando interno `cd`.
- Redirecciones de entrada y salida con `parse_redir.h`.
- Ejecucion en foreground y background.
- Gestion de trabajos con `jobs`, `bg` y `fg`.
- Consulta y borrado del trabajo actual con `currjob` y `deljob`.
- Listado de procesos zombie con `zjobs`.
- Lanzamiento multiple en background con `bgteam`.
- Trabajo respawnable usando `+` al final del comando.
- Tiempo maximo de vida con `alarm-thread`.
- Ejecucion diferida con `delay-thread`.
- Enmascaramiento de senales con `mask`.
- Control experimental del manejador de `SIGCHLD` con `desactivar`, `activar` y `limpiar`.

## Estructura

```text
.
├── Shell_project.c      # Bucle principal de la shell y comandos internos
├── job_control.c        # Utilidades de jobs, parsing basico y senales
├── job_control.h        # Tipos, macros y cabeceras de job control
├── parse_redir.h        # Parser auxiliar de redirecciones
├── LEEME-original.txt   # Explicacion original de ampliaciones
├── Makefile             # Compilacion en Linux/WSL
├── Dockerfile           # Ejecucion reproducible con Docker
├── .dockerignore        # Archivos excluidos del contexto Docker
└── .gitignore           # Archivos generados localmente
```

## Opcion recomendada: probar con Docker

Docker permite ejecutar el proyecto en un entorno Linux aunque estes en Windows. La primera ejecucion puede tardar porque Docker tiene que descargar la imagen base y compilar el binario. Las siguientes ejecuciones suelen ser mucho mas rapidas gracias a la cache.

### 1. Instalar Docker si no lo tienes

#### Windows

La forma recomendada en Windows es instalar Docker Desktop con backend WSL 2.

1. Instala WSL si aun no lo tienes:

```powershell
wsl --install
```

2. Reinicia Windows si el instalador de WSL lo pide.
3. Descarga e instala Docker Desktop desde la documentacion oficial:
   <https://docs.docker.com/desktop/setup/install/windows-install/>
4. Durante la instalacion, usa WSL 2 como backend para contenedores Linux.
5. Abre Docker Desktop y espera a que indique que Docker esta arrancado.
6. Comprueba la instalacion desde PowerShell:

```powershell
docker version
docker run hello-world
```

Notas para Windows:

- Debes tener la virtualizacion activada en BIOS/UEFI.
- Docker Desktop ejecuta los contenedores Linux dentro de una VM/WSL 2.
- No hace falta compilar un `.exe` de Windows para probar este proyecto.
- Usa PowerShell, Windows Terminal o la terminal integrada de VS Code.

#### Linux

En Linux puedes instalar Docker Engine directamente. En Ubuntu, la instalacion oficial usa el repositorio de Docker:

```bash
sudo apt update
sudo apt install ca-certificates curl
sudo install -m 0755 -d /etc/apt/keyrings
sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
sudo chmod a+r /etc/apt/keyrings/docker.asc
sudo tee /etc/apt/sources.list.d/docker.sources <<EOF
Types: deb
URIs: https://download.docker.com/linux/ubuntu
Suites: $(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}")
Components: stable
Architectures: $(dpkg --print-architecture)
Signed-By: /etc/apt/keyrings/docker.asc
EOF
sudo apt update
sudo apt install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
```

Comprueba que funciona:

```bash
sudo docker run hello-world
```

Si quieres usar `docker` sin escribir `sudo` cada vez:

```bash
sudo groupadd docker
sudo usermod -aG docker $USER
newgrp docker
docker run hello-world
```

Notas para Linux:

- En Linux Docker corre de forma mas directa que en Windows, sin Docker Desktop ni WSL.
- Si no anades tu usuario al grupo `docker`, usa `sudo docker ...` en los comandos.
- El grupo `docker` da permisos altos sobre el sistema; usalo solo en tu maquina de desarrollo.
- Para Debian, Fedora u otras distribuciones, revisa la guia oficial de tu distro: <https://docs.docker.com/engine/install/>

### 2. Construir la imagen

Desde la carpeta del proyecto:

Windows PowerShell:

```powershell
cd C:\Users\aleja\Documents\GitHub\ShellLinux
docker build -t shell-linux .
```

Linux, WSL o macOS:

```bash
cd ~/ShellLinux
docker build -t shell-linux .
```

Si estas en Linux y no configuraste el grupo `docker`, usa:

```bash
sudo docker build -t shell-linux .
```

### 3. Ejecutar la shell

Windows PowerShell:

```powershell
docker run --rm -it shell-linux
```

Linux, WSL o macOS:

```bash
docker run --rm -it shell-linux
```

Si estas en Linux sin grupo `docker`:

```bash
sudo docker run --rm -it shell-linux
```

El flag `-it` es importante porque la shell necesita una terminal interactiva para probar correctamente el control de procesos. El flag `--rm` borra el contenedor al salir, asi no se quedan contenedores parados ocupando espacio.

Para salir de la shell, escribe `exit` si lo tienes implementado o pulsa `Ctrl+D`.

## Probar sin Docker

### Linux o WSL

Instala compilador y herramientas basicas:

```bash
sudo apt update
sudo apt install build-essential
```

Compila y ejecuta:

```bash
make
./shell
```

Tambien puedes compilar sin Makefile:

```bash
gcc Shell_project.c job_control.c -o shell -pthread
./shell
```

Limpia los archivos generados:

```bash
make clean
```

### Windows nativo

No es la opcion recomendada. El proyecto depende de llamadas POSIX de Linux, por lo que no funcionara como `.exe` normal de Windows sin adaptar bastante codigo. Para Windows, usa Docker o WSL.

## Ejemplos de uso

Dentro de la shell veras el prompt:

```text
COMMAND->
```

Comandos basicos:

```bash
ls
pwd
cd /tmp
echo hola desde la shell
```

Procesos en segundo plano y jobs:

```bash
sleep 10 &
jobs
fg 1
bg 1
```

Redirecciones:

```bash
echo hola > salida.txt
cat < salida.txt
```

Trabajo respawnable:

```bash
sleep 3 +
```

Tiempo maximo de vida:

```bash
alarm-thread 5 sleep 20
```

Ejecucion diferida:

```bash
delay-thread 3 echo hola
```

Lanzamiento multiple en background:

```bash
bgteam 3 sleep 10
```

Enmascaramiento de senales:

```bash
mask 2 sleep 10
```

Otros comandos internos disponibles:

```bash
currjob
deljob
zjobs
parar
desactivar
activar
limpiar
```
