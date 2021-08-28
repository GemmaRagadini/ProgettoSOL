sleep 5

cd serverdir
./server config_server_test2.txt &

SERVER_PID=$!
export SERVER_PID
bash -c 'sleep 5 && kill -1 ${SERVER_PID}' &
TIMER_PID=$!

sleep 0.5

cd ../clientdir
CWD=$(pwd)

echo "Provo l'algoritmo di rimpiazzamento dei file:"

echo "Scrivo tutti i file presenti nella directory $CWD/provafile"
echo "Running client -t 500 -p -w $CWD/provafile"
./client -t 500 -p -w "$CWD/provafile"
sleep .2

echo "Lettura di tutti i file e salvataggio in una directory:"
echo "Running client -t 500 -p -d $(pwd)/dir3 -R 0"
./client -t 500 -p -d "$(pwd)/dir3" -R 0
sleep 0.2

wait $TIMER_PID
wait $SERVER_PID