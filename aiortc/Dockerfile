FROM python:latest
WORKDIR /app
COPY ["aiortc", ""]
COPY ["html", "../html/"]
RUN pip install aiohttp aiortc
EXPOSE 8080
ENTRYPOINT python server.py
