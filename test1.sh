sleep 5

cd serverdir
valgrind --leak-check=full ./server &

cd ../clientdir
CWD=$(pwd)


echo "Running client -t 500 -p -W $CWD/provafile/read"
./client -t 500 -p -W "$CWD/provafile/read"
sleep .2

echo "Running client -t 500 -p -w $CWD/provafile"
./client -t 500 -p -w "$CWD/provafile"
sleep .2

echo "Running client -t 500 -p -r $CWD/provafile/read -d $CWD/dir"
./client -t 500 -p -d "$CWD/dir" -r "$CWD/provafile/read"
sleep .2

echo "Running client -t 500 -p -w $CWD/provafile -d $CWD/dir -R 0"
./client -t 500 -p -w "$CWD/provafile" -d "$CWD/dir" -R 0
sleep .2

kill -s HUP $(ps aux | grep sol_server | grep -v grep | awk -F " " '{print $2}')
