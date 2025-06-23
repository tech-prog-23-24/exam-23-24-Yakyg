# QT TCP Server (Fedora-based)
FROM fedora:36

LABEL maintainer="Мных Ян 241-3211 mnych09@mail.com"

# Установим Qt5 и компилятор
RUN dnf -y update && \
    dnf -y install qt5-qtbase-devel gcc-c++ make

# Создаем рабочую папку
RUN mkdir -p /var/lib/server
WORKDIR /var/lib/server

# Копируем проект
COPY . /var/lib/server

# Собираем сервер 
RUN qmake-qt5 echoServer.pro
RUN make
# Открываем порт 65432
EXPOSE 65432

# Запускаем сервер при старте контейнера
CMD ["./echoServer"]
