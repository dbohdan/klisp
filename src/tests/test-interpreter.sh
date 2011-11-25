#! /bin/sh
#
# Test of the stand-alone interpreter.
#

if [ $# -ne 1 ] ; then
    echo "usage: test-interpreter.sh KLISP-EXECUTABLE" 1>&2
    exit 1
fi

KLISP="$1"
GEN_K="test-interpreter-gen.k"

# -- functions ----------------------------------------

init()
{
    nfail=0
    npass=0
}

check_helper()
{
    expected_output="$1"
    expected_exitstatus="$2"
    output="$3"
    exitstatus="$4"
    command="$5"

    if [ "$output" != "$expected_output" ] ; then
        echo "FAIL: $command"
        echo "  =======> $output"
        echo " expected: $expected_output" 1>&2
        nfail=$((1 + nfail))
    elif [ $exitstatus -ne $expected_exitstatus ] ; then
        echo "FAIL: $command"
        echo "  ==> exit status $exitstatus ; expected: $expected_exitstatus" 1>&2
        nfail=$((1 + nfail))
    else
        ## echo "OK: $command ==> $output"
        npass=$((1 + npass))
    fi
}

check_o()
{
    expected_output="$1"
    shift
    o=`"$@"`
    s=$?
    check_helper "$expected_output" 0 "$o" "$s" "$*"
}

check_os()
{
    expected_output="$1"
    expected_exitstatus="$2"
    shift
    shift
    o=`"$@"`
    s=$?
    check_helper "$expected_output" "$expected_exitstatus" "$o" "$s" "$*"
}

check_io()
{
    expected_output="$1"
    input="$2"
    shift
    shift
    o=`echo "$input" | "$@"`
    s=$?
    check_helper "$expected_output" 0 "$o" "$s" "$*"
}

report()
{
    echo "Tests Passed: $npass"
    echo "Tests Failed: $nfail"
    echo "Tests Total: $((npass + nfail))"
}

# -- tests --------------------------------------------

init

# script name on the command line

echo '(display 123456)' > "$GEN_K"
check_o '123456' $KLISP "$GEN_K"
rm "$GEN_K"

# '-' on the command line

check_io '2' '(display (+ 1 1))' $KLISP -

# option: -e

check_o 'abcdef' $KLISP '-e (display "abc")' '-e' '(display "def")'

# '--' on the command line

check_o '1' $KLISP '-e (display 1)' --

# exit status

check_os '' 0 $KLISP -e '(exit 0)'
check_os '' 1 $KLISP -e '(exit 1)'
check_os '' 2 $KLISP -e '(exit 2)'
check_os '' 0 $KLISP -e '(exit #t)'
check_os '' 1 $KLISP -e '(exit #f)'
check_os '' 0 $KLISP -e '(exit #inert)'
check_os '' 1 $KLISP -e '(exit ())'
check_os '' 0 $KLISP -e '(exit)'
check_os '' 0 $KLISP -e '1'
check_os '' 3 $KLISP -e '(apply-continuation root-continuation 3)'

# KLISP_INIT environment variable

export KLISP_INIT='(display "init...")'
check_o 'init...main' $KLISP -e '(display "main")'
export KLISP_INIT=

# other environment variables

export KLISPTEST1=pqr
check_o '"pqr"' $KLISP '-e (write (get-environment-variable "KLISPTEST1"))'
check_o '#f' $KLISP '-e (write (get-environment-variable "KLISPTEST2"))'

# script arguments

check_o '()' $KLISP -e '(write(get-script-arguments))'
check_io '("-" "-i")' '' $KLISP -e '(write(get-script-arguments))' - -i
check_o '("/dev/null" "y")' $KLISP -e '(write(get-script-arguments))' /dev/null y
check_o '()' $KLISP -e '(write(get-script-arguments))' --
check_o '("/dev/null")' $KLISP -e '(write(get-script-arguments))' -- /dev/null

# interpreter arguments
#  (get-interpreter-arguments) returns all command line
#  arguments.
#
#  TODO: The man page says that (interpreter-arguments)
# returns the arguments _before_ the script name.
#

check_o "(\"$KLISP\" \"-e\" \"(write(get-interpreter-arguments))\")" \
    $KLISP -e '(write(get-interpreter-arguments))'
check_o "(\"$KLISP\" \"-e\" \"(write(get-interpreter-arguments))\" \"--\")" \
    $KLISP -e '(write(get-interpreter-arguments))' --
check_io "(\"$KLISP\" \"-e\" \"(write(get-interpreter-arguments))\" \"-\")" '' \
    $KLISP -e '(write(get-interpreter-arguments))' -
check_o "(\"$KLISP\" \"-e\" \"(write(get-interpreter-arguments))\" \"/dev/null\")" \
    $KLISP -e '(write(get-interpreter-arguments))' /dev/null
check_o "(\"$KLISP\" \"-e(write(get-interpreter-arguments))\" \"--\" \"/dev/null\")" \
    $KLISP '-e(write(get-interpreter-arguments))' -- /dev/null
check_o "(\"$KLISP\" \"-e(write(get-interpreter-arguments))\" \"--\" \"/dev/null\" \"a\" \"b\" \"c\")" \
    $KLISP '-e(write(get-interpreter-arguments))' -- /dev/null a b c

# done

report
