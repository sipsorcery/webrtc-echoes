FROM python:latest
WORKDIR /app
COPY ["aiortc", ""]
COPY ["html", "../html/"]
COPY ["aiortc/client.sh", "/client.sh"]
RUN chmod +x /client.sh
RUN pip install aiohttp aiortc
EXPOSE 8080
ENTRYPOINT python server.py
