if grep -qi microsoft /proc/version 2>/dev/null; then
    # echo "Running on WSL"
	$(dirname $0)/../../bin/asteroids.exe
else
    # echo "Not WSL"
	$(dirname $0)/../../bin/asteroids
fi
