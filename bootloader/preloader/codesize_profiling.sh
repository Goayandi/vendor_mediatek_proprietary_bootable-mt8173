INPUT_FILE=$1
OUTPUT_FILE=$2

echo "Symbol,Address,Size,Object" > codesize_profiling_temp.csv
awk '/^ .text/ {
    if ($2 != "" && $2 >= "0x00200000") {
        $4 = substr($4, match($4, /[^/]+$/));
        print $1 "," $2 "," strtonum($3) "," $4;
    }
}' ${INPUT_FILE} >> codesize_profiling_temp.csv

awk '/^ .text/ {
    if ($2 == "") {
        A = $1;
        getline;
        $3 = substr($3, match($3, /[^/]+$/));
        A = A "," $1 "," strtonum($2) "," $3;
        if ($1 >= "0x00200000")
            print A;
        A = "";
    }
}' ${INPUT_FILE} >> codesize_profiling_temp.csv

awk '/^ .data/ {
    if ($2 != "" && $2 >= "0x00200000") {
        $4 = substr($4, match($4, /[^/]+$/));
        print $1 "," $2 "," strtonum($3) "," $4;
    }
}' ${INPUT_FILE} >> codesize_profiling_temp.csv

awk '/^ .data/ {
    if ($2 == "") {
        A = $1;
        getline;
        $3 = substr($3, match($3, /[^/]+$/));
        A = A "," $1 "," strtonum($2) "," $3;
        if ($1 >= "0x00200000")
            print A;
        A = "";
    }
}' ${INPUT_FILE} >> codesize_profiling_temp.csv

awk '/^ .rodata/ {
    if ($2 != "" && $2 >= "0x00200000") {
        $4 = substr($4, match($4, /[^/]+$/));
        print $1 "," $2 "," strtonum($3) "," $4;
    }
}' ${INPUT_FILE} >> codesize_profiling_temp.csv

awk '/^ .rodata/ {
    if ($2 == "") {
        A = $1;
        getline;
        $3 = substr($3, match($3, /[^/]+$/));
        A = A "," $1 "," strtonum($2) "," $3;
        if ($1 >= "0x00200000")
            print A;
            A = "";
    }
}' ${INPUT_FILE} >> codesize_profiling_temp.csv

awk '/^ \*\* merge strings/ {
    A = $1" "$2" "$3;
    getline;
    A = A "," $1 "," strtonum($2);
    print A;
}' ${INPUT_FILE} >> codesize_profiling_temp.csv

awk '/^.got/ {
    print $1 "," $2 "," strtonum($3);
}' ${INPUT_FILE} >> codesize_profiling_temp.csv

awk '
    BEGIN {
        FS = ",";
        print "Object,Symbol,Size";
    }

    {
        if (NR > 1) {
            obj_sym = $4 " " $1;    # use object + symbol as key
            object[obj_sym] = $4;
            size[obj_sym] = $3;
            total += $3;
        }
    }

    END {
            for (obj_sym in object) {
                sum[object[obj_sym]] += size[obj_sym];
            }

            PROCINFO["sorted_in"] = "@val_num_desc"
            for (filename in sum) {
                print filename",," sum[filename];

                for (obj_sym in object) {
                    if (object[obj_sym] == filename) {
                        symbol = substr(obj_sym, match(obj_sym, / /) + 1);
                        print "," symbol "," size[obj_sym]
                    }
                }
            }

            print "Total,," total;
    }
' codesize_profiling_temp.csv >> ${OUTPUT_FILE}

rm codesize_profiling_temp.csv
echo "End of Profiling..."
