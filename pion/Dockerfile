FROM golang:latest AS build
WORKDIR /src
COPY ["pion", ""]
RUN go build main.go

FROM golang:latest AS final
WORKDIR /app
COPY --from=build /src/main .
COPY ["html", "../html/"]
EXPOSE 8080
ENTRYPOINT /app/main