#!/bin/sh
PIDFILE=/var/run/test-service.pid
echo $$ > "$PIDFILE"
logger -t test-service "Test service started (pid=$$)"
while true; do
    sleep 5
    logger -t test-service "Test service alive (pid=$$)"
done
