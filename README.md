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

Para salir de la shell, pulsa `Ctrl+D`.

> Importante: esta shell separa argumentos por espacios y no implementa tuberias ni operadores de Bash como `|`, `;`, `&&` o `||`. Para tener varios procesos ocurriendo a la vez, usa `&`, `bgteam`, `delay-thread`, `+`, `jobs`, `fg` y `bg`.

### Comandos externos del sistema

Cualquier programa disponible dentro del sistema Linux se puede ejecutar como comando normal:

```bash
pwd
ls
ls -la
echo hola desde la shell
ps
sleep 2
```

### `cd`

Cambia el directorio actual. Si no pasas ruta, intenta volver al directorio `HOME`.

```bash
cd /tmp
pwd
cd
pwd
```

### Redirecciones `<` y `>`

Permiten leer entrada desde un archivo o escribir la salida en un archivo.

Debe haber espacios antes y despues de `<` o `>`.

```bash
echo hola > salida.txt
cat < salida.txt
ps > procesos.txt
cat < procesos.txt
```

Tambien se pueden combinar con otras funciones del shell:

```bash
sleep 2 > terminado.txt &
delay-thread 3 echo delayed > delayed.txt
```

### Background con `&`

Ejecuta un comando en segundo plano y devuelve el prompt inmediatamente.

```bash
sleep 20 &
echo puedo seguir escribiendo mientras sleep sigue vivo
jobs
```

Puedes lanzar varios procesos en background escribiendo varios comandos, uno por linea:

```bash
sleep 30 &
sleep 40 &
sleep 50 &
jobs
```

### `jobs`

Muestra los trabajos que la shell tiene guardados en su lista interna.

```bash
sleep 30 &
jobs
```

### `currjob`

Muestra el trabajo actual, que es el primero de la lista de jobs.

```bash
sleep 30 &
sleep 40 &
currjob
```

### `deljob`

Borra el trabajo actual de la lista si no esta suspendido.

```bash
sleep 30 &
jobs
currjob
deljob
jobs
```

### `fg [posicion]`

Trae un job al primer plano. Si no indicas posicion, usa la posicion `1`.

```bash
sleep 30 &
jobs
fg 1
```

Tambien puedes suspender un proceso en primer plano con `Ctrl+Z` y despues volver a traerlo:

```bash
sleep 60
# Pulsa Ctrl+Z
jobs
fg 1
```

### `bg [posicion]`

Reanuda en segundo plano un job suspendido. Si no indicas posicion, usa la posicion `1`.

```bash
sleep 60
# Pulsa Ctrl+Z
jobs
bg 1
jobs
```

### `parar`

Envia `SIGSTOP` a los trabajos en background que esten registrados en la lista de jobs.

```bash
sleep 60 &
sleep 70 &
jobs
parar
jobs
bg 1
jobs
```

### `zjobs`

Lista procesos zombie encontrados recorriendo `/proc`.

```bash
zjobs
```

Normalmente puede no mostrar nada si no hay procesos zombie en ese momento.

### `bgteam N comando`

Lanza `N` copias de un comando en background.

```bash
bgteam 3 sleep 20
jobs
```

Ejemplo para ver varios procesos a la vez:

```bash
bgteam 4 sleep 30
jobs
ps
```

### Respawnable con `+`

Si un comando termina con `+`, se guarda como respawnable. Cuando el proceso acaba, la shell lo vuelve a lanzar automaticamente hasta que lo pases a `fg` o `bg` y termine fuera del modo respawnable.

```bash
sleep 3 +
jobs
```

Espera unos segundos y vuelve a consultar:

```bash
jobs
```

### `alarm-thread SEGUNDOS comando`

Ejecuta un comando y lo mata si sigue vivo despues de los segundos indicados.

```bash
alarm-thread 5 sleep 20
```

Ejemplo donde no deberia matar nada porque el comando termina antes:

```bash
alarm-thread 5 sleep 1
```

### `delay-thread SEGUNDOS comando`

Espera los segundos indicados y luego lanza el comando en background.

```bash
delay-thread 3 echo hola con retraso
delay-thread 5 sleep 20
jobs
```

Ejemplo con varios eventos programados:

```bash
delay-thread 2 echo primero
delay-thread 4 echo segundo
delay-thread 6 echo tercero
```

### `mask SENAL... -c comando`

Ejecuta un comando bloqueando las senales indicadas en el proceso hijo. Las senales se pasan por numero y despues se escribe `-c` seguido del comando real.

```bash
mask 2 -c sleep 20
mask 2 15 -c sleep 20
```

Ejemplo con background:

```bash
mask 2 15 -c sleep 40 &
jobs
```

### `desactivar`

Desactiva el manejador de `SIGCHLD` de la shell y activa el comportamiento experimental de autokill/limpieza definido en el proyecto.

```bash
desactivar
sleep 2 &
jobs
```

### `activar`

Restaura el manejador de `SIGCHLD`.

```bash
activar
```

### `limpiar`

Fuerza una pasada del manejador de limpieza de hijos terminados.

```bash
sleep 1 &
# Espera un par de segundos
limpiar
jobs
```

## Secuencias para probar varias cosas a la vez

Copia estas lineas una a una dentro de la shell para ver varios mecanismos funcionando al mismo tiempo.

### Jobs en paralelo, parada y reanudacion

```bash
sleep 60 &
sleep 70 &
bgteam 2 sleep 80
jobs
parar
jobs
bg 1
fg 1
```

### Delays, redirecciones y jobs

```bash
delay-thread 2 echo primer delay > primer-delay.txt
delay-thread 4 echo segundo delay > segundo-delay.txt
sleep 5
sleep 10 &
jobs
cat < primer-delay.txt
cat < segundo-delay.txt
```

### Respawnable y control manual

```bash
sleep 3 +
jobs
# Espera 5 o 6 segundos
jobs
bg 1
jobs
```

### Alarmas y background

```bash
alarm-thread 3 sleep 20
sleep 30 &
alarm-thread 5 sleep 10
jobs
```

### Mask con procesos largos

```bash
mask 2 15 -c sleep 30 &
jobs
parar
jobs
bg 1
```
