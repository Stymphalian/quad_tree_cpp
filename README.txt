docker build -t ak-chibi-bot -f Dockerfile --target development .
docker build -t stymphalian/ak-chibi-bot:prod -f Dockerfile --target production --platform linux/x86_64 .
docker build -t ak-chibi-bot:prod -f Dockerfile --target production --platform linux/x86_64 .
docker build -t ak-db -f Dockerfile --target development .
docker build -t ak-db:prod -f Dockerfile --target production .
docker compose -f compose.yaml up -d
docker compose -f compose.yaml down
docker compose -f compose.yaml logs
docker exec -it ak-services-db-1 bash
docker exec -it ak-service-web-1 base
docker run -it --rm -v .:/app -v .\\secrets:/run/secrets -p 8080:8080 ak-chibi-bot
go run main.go -image_assetdir=/ak_chibi_assets/assets -address=:8080 -bot_config=dev_config.json
go clean -testcache && go test ./...
docker exec -it ak-services-db-1 tail -f /var/lib/postgresql/data/logs/postgresql-Wed.log

cd build
cmake ..

docker build -t removepause .
docker run -it --rm -v C:/Users/Jordan/Desktop/dev/lab/remove_pause/cprog/vscode_docker:/work removepause
docker run -it --rm -v C:/Users/Jordan/Desktop/dev/lab/remove_pause/cprog/vscode_docker:/work linux-cpp-dev

docker run -it -v C:/Users/Jordan/Desktop/dev/lab/remove_pause/cprog/vscode_docker:/work --name linux-cpp-dev-container build/noin "vod2.mkv mask.png output3.mkv"

docker run -it --rm -v C:/Users/Jordan/Desktop/dev/lab/remove_pause/cprog/vscode_docker:/work removepause build/noin paused.mkv mask_1920_1080.png pauseless.mkv
docker run -it --rm -v C:/Users/Jordan/Desktop/dev/lab/remove_pause/cprog/vscode_docker:/work removepause build/noin paused.mkv mask_2560_1440.png pauseless.mkv

g++ -std=c++20 -O3 -o build/noin build/noin.cpp && ./build/noin
g++ -std=c++20 -O3 -o build/noin src/main.cpp && ./build/noin


g++ -std=c++20 -O3 -o noin.exe src/main.cpp
