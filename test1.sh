sleep 5

cd serverdir
valgrind --leak-check=full ./server &

SERVER_PID=$!
export SERVER_PID
bash -c 'sleep 5 && kill -1 ${SERVER_PID}' &
TIMER_PID=$!

sleep 0.5
cd ../clientdir

echo "Scrittura di un file:"
echo "Running client -t 500 -p -W $(pwd)/provafile/read"
./client -t 500 -p -W "$(pwd)/provafile/read"
sleep 0.2

echo "Scrittura di tutti i file presenti in una directory:"
echo "Running client -t 500 -p -w $(pwd)/provafile"
./client -t 500 -p -w "$(pwd)/provafile"
sleep 0.2

echo "Lettura di un file e salvataggio in una directory:"
echo "Running client -t 500 -p -r $(pwd)/provafile/read -d $(pwd)/dir1"
./client -t 500 -p -d "$(pwd)/dir1" -r "$(pwd)/provafile/read"
sleep 0.2

echo "Scrittura di tutti i file e lettura di 3 file con salvataggio in una directory:"
echo "Running client -t 500 -p -w $(pwd)/provafile -d $(pwd)/dir1 -R 3"
./client -t 500 -p -w "$(pwd)/provafile" -d "$(pwd)/dir1" -R 3
sleep 0.2

echo "Lettura di tutti i file e salvataggio in una directory:"
echo "Running client -t 500 -p -d $(pwd)/dir2 -R 0"
./client -t 500 -p -d "$(pwd)/dir2" -R 0
sleep 0.2

wait $TIMER_PID
wait $SERVER_PID

