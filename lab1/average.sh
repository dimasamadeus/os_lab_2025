#!/bin/bash
echo "$@" | awk '{
    for(i=1;i<=NF;i++) {
        sum += $i
        count++
    }
}
END {
    print "Количество:", count
    print "Среднее:", sum/count
}'