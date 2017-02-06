#! /bin/bash

if [ -z $DBMD ]; then
    DBMD=database.md
fi

state=0

cat $DBMD | tr -d "\r" |  while read line
do
    field=${line%%\ *}
    if [ "$field" == '#' ]; then
        if [ $state -eq 0 ]; then
            dbname=${line#*\ }
            echo "CREATE DATABASE IF NOT EXISTS \`$dbname\` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;"
	    echo
	    echo "USE \`$dbname\`;"
            state=1
        else
            exit 1
        fi
    elif [ "$field" == '##' ]; then
        if [ "${line#*\ }" == 'tables' ]; then
            if [ $state -eq 1 ]; then
                state=2
            else
                exit 1
            fi
        elif [ "${line#*\ }" == 'sql' ]; then
            if [[ $state == 2 ]] || [[ $state == 1 ]]; then
                state=5
            else
                exit 1
            fi
        fi
    elif [ "$field" == '###' ]; then
        if [ $state -eq 2 ]; then
            echo
            echo "CREATE TABLE IF NOT EXISTS \`$DBPREF${line#*\ }\` ("
            state=3
            unset primkey
        else
            exit 1
        fi
    elif [ "$field" == '```' ]; then
        if [ $state -eq 3 ]; then
            state=4
        elif [ $state -eq 4 ]; then
            if [ ! -z $primkey ]; then
                echo "  PRIMARY KEY (\`$primkey\`)";
            fi
            echo ")$TBLATTR;"
            state=2
        elif [ $state -eq 5 ]; then
            echo
            state=6
        elif [ $state -eq 6 ]; then
            state=1
        else
            exit 1
        fi
    elif [ $state -eq 4 ]; then
        attr='NOT NULL'
        if [ -z $primkey ]; then
            primkey=$field
            if [ "$field" == 'id' ]; then
                attr=$attr' AUTO_INCREMENT'
            fi
        fi
        temp=${line#*\ }
        typename=${temp%\ //*}
        case $typename in
            'int') typename='int(11)';;
            'tinyint') typename='tinyint(4)';;
        esac
        comment=${line#*//\ }
        echo "  \`$field\` $typename $attr COMMENT '$comment',"
    elif [ $state -eq 6 ]; then
        echo "$line"
    fi
done

