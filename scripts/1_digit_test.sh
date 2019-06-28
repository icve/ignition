
HOST='192.168.4.1'
PORT=6666
WAIT_PERIOD='0.1'

echo 's0-----\n\0' | nc $HOST $PORT
sleep $WAIT_PERIOD
echo 's1-----\n' | nc $HOST $PORT
sleep $WAIT_PERIOD
echo 's2-----\n' | nc $HOST $PORT
sleep $WAIT_PERIOD
echo 's3-----\n' | nc $HOST $PORT
sleep $WAIT_PERIOD
echo 's4-----\n' | nc $HOST $PORT
sleep $WAIT_PERIOD
echo 's5-----\n' | nc $HOST $PORT
sleep $WAIT_PERIOD
echo 's6-----\n' | nc $HOST $PORT
sleep $WAIT_PERIOD
echo 's7-----\n' | nc $HOST $PORT
sleep $WAIT_PERIOD
echo 's8-----\n' | nc $HOST $PORT
sleep $WAIT_PERIOD
echo 's9-----\n' | nc $HOST $PORT
sleep $WAIT_PERIOD
