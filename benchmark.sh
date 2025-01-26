#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

# Test shells
SHELLS=("./build/amcsh" "/bin/bash" "/bin/zsh")
ITERATIONS=1000

# Function to get current time in milliseconds
get_time_ms() {
    perl -MTime::HiRes=time -e 'printf "%.0f\n", time * 1000'
}

echo -e "${BLUE}Starting shell performance benchmark...${NC}\n"

# 1. Command Execution Speed Test
echo -e "${GREEN}1. Testing basic command execution speed (echo 'test')${NC}"
for shell in "${SHELLS[@]}"; do
    shell_name=$(basename $shell)
    start=$(get_time_ms)
    for ((i=0; i<ITERATIONS; i++)); do
        echo "echo test" | $shell >/dev/null 2>&1
    done
    end=$(get_time_ms)
    duration=$((end - start))
    echo "$shell_name: $duration ms for $ITERATIONS iterations"
done
echo

# 2. Pipeline Performance Test
echo -e "${GREEN}2. Testing pipeline performance (ls | grep a)${NC}"
for shell in "${SHELLS[@]}"; do
    shell_name=$(basename $shell)
    start=$(get_time_ms)
    for ((i=0; i<100; i++)); do
        echo "ls -la /usr/bin | grep a" | $shell >/dev/null 2>&1
    done
    end=$(get_time_ms)
    duration=$((end - start))
    echo "$shell_name: $duration ms for 100 iterations"
done
echo

# 3. Startup Time Test
echo -e "${GREEN}3. Testing shell startup time${NC}"
for shell in "${SHELLS[@]}"; do
    shell_name=$(basename $shell)
    start=$(get_time_ms)
    for ((i=0; i<100; i++)); do
        $shell -c "exit" >/dev/null 2>&1
    done
    end=$(get_time_ms)
    duration=$((end - start))
    echo "$shell_name: $duration ms for 100 iterations"
done
echo

# 4. Command Cache Performance
echo -e "${GREEN}4. Testing command cache performance${NC}"
TEST_CMD="ls -la /usr/bin"
for shell in "${SHELLS[@]}"; do
    shell_name=$(basename $shell)
    echo "First run of $TEST_CMD in $shell_name:"
    start=$(get_time_ms)
    echo "$TEST_CMD" | $shell >/dev/null 2>&1
    end=$(get_time_ms)
    first_duration=$((end - start))
    echo "Duration: $first_duration ms"
    
    echo "Second run of $TEST_CMD in $shell_name (should be faster for amcsh):"
    start=$(get_time_ms)
    echo "$TEST_CMD" | $shell >/dev/null 2>&1
    end=$(get_time_ms)
    second_duration=$((end - start))
    echo "Duration: $second_duration ms"
    echo
done

# 5. Multiple Background Jobs Test
echo -e "${GREEN}5. Testing multiple background jobs${NC}"
for shell in "${SHELLS[@]}"; do
    shell_name=$(basename $shell)
    start=$(get_time_ms)
    echo "sleep 0.1 & sleep 0.1 & sleep 0.1 & sleep 0.1 & wait" | $shell >/dev/null 2>&1
    end=$(get_time_ms)
    duration=$((end - start))
    echo "$shell_name: $duration ms for running 4 parallel sleep commands"
done
echo

# 6. Memory Usage Test
echo -e "${GREEN}6. Testing memory usage${NC}"
for shell in "${SHELLS[@]}"; do
    shell_name=$(basename $shell)
    echo "Memory usage for $shell_name:"
    /usr/bin/time -l $shell -c "exit" 2>&1 | grep "maximum resident set size"
done
