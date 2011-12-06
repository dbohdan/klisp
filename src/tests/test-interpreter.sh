#! /bin/sh
#
# Test of the stand-alone interpreter.
#
# Does not work in MSYS shell on Windows because
# of different handling of command line arguments.
# TODO: Write similar test script for Windows cmd.exe or PowerShell.
#

if [ $# -ne 1 ] ; then
    echo "usage: test-interpreter.sh KLISP-EXECUTABLE" 1>&2
    exit 1
fi

KLISP="$1"
GEN_K="test-interpreter-gen.k"
TMPERR="test-interpreter-err.log"

# -- functions ----------------------------------------

init()
{
    nfail=0
    npass=0
}

check_match()
{
    expected="$1"
    actual="$2"

    if regexp=`expr match "$expected" '/\(.*\)/$'` ; then
        expr match "$actual" "$regexp" >/dev/null
    else
        test "$actual" = "$expected"
    fi
}

check_helper()
{
    expected_stdout="$1"
    expected_stderr="$2"
    expected_exitstatus="$3"
    stdout="$4"
    stderr="$5"
    exitstatus="$6"
    command="$7"

    if ! check_match "$expected_stdout" "$stdout" ; then
        echo "FAIL: $command"
        echo " stdout => '$stdout'"
        echo " expected: '$expected_stdout'" 1>&2
        nfail=$((1 + $nfail))
    elif ! check_match "$expected_stderr" "$stderr" ;  then
        echo "FAIL: $command"
        echo " stderr => '$stderr'"
        echo " expected: '$expected_stderr'" 1>&2
        nfail=$((1 + $nfail))
    elif [ $exitstatus -ne $expected_exitstatus ] ; then
        echo "FAIL: $command"
        echo "  ==> exit status $exitstatus ; expected: $expected_exitstatus" 1>&2
        nfail=$((1 + $nfail))
    else
        ## echo "OK: $command ==> $stdout"
        npass=$((1 + $npass))
    fi
}

check_o()
{
    expected_output="$1"
    shift
    o=`"$@" 2> $TMPERR`
    s=$?
    e=`cat $TMPERR`
    check_helper "$expected_output" '' 0 "$o" "$e" "$s" "$*"
}

check_os()
{
    expected_output="$1"
    expected_exitstatus="$2"
    shift
    shift
    o=`"$@" 2> $TMPERR`
    s=$?
    e=`cat $TMPERR`
    check_helper "$expected_output" '' "$expected_exitstatus" "$o" "$e" "$s" "$*"
}

check_oi()
{
    expected_output="$1"
    input="$2"
    shift
    shift
    o=`echo "$input" | "$@" 2> $TMPERR`
    s=$?
    e=`cat $TMPERR`
    check_helper "$expected_output" '' 0 "$o" "$e" "$s" "echo '$input' | $*"
}

check_oe()
{
    expected_stdout="$1"
    expected_stderr="$2"
    shift
    shift
    o=`"$@" 2> $TMPERR`
    s=$?
    e=`cat $TMPERR`
    check_helper "$expected_stdout" "$expected_stderr" 0 "$o" "$e" "$s" "$*"
}

report()
{
    echo "Tests Passed: $npass"
    echo "Tests Failed: $nfail"
    echo "Tests Total: $(($npass + $nfail))"
}

cleanup()
{
    rm -f "$GEN_K" "$TMPERR"
}
# -- tests --------------------------------------------

init

# script name on the command line

echo '(display 123456)' > "$GEN_K"
check_o '123456' $KLISP "$GEN_K"

# empty command line and stdin not a terminal

check_oi '' '' $KLISP

# '-' on the command line

check_oi '2' '(display (+ 1 1))' $KLISP -

# option: -e

check_o 'abcdef' $KLISP '-e (display "abc")' '-e' '(display "def")'

# option: -i
# The interpreter always show name and version
# WAS check_oi 'klisp> ' '' $KLISP -i

check_oi '/klisp [0-9.][0-9.]* .*\n.*klisp> /' '' $KLISP -i

# option: -v

check_o '/klisp [0-9.][0-9.]* .*/' $KLISP -v

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

## FIX the root continuation should exit without running any more 
## arguments, but it doesn't...
check_os '' 0 $KLISP -e '(exit 0)' -e '(exit 1)'
check_os '' 1 $KLISP -e '(exit 1)' -e '(exit 0)'

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
check_oi '("-" "-i")' '' $KLISP -e '(write(get-script-arguments))' - -i
check_o '("/dev/null" "y")' $KLISP -e '(write(get-script-arguments))' /dev/null y
check_o '()' $KLISP -e '(write(get-script-arguments))' --
check_o '("/dev/null")' $KLISP -e '(write(get-script-arguments))' -- /dev/null

# interpreter arguments
#  (get-interpreter-arguments) returns all command line
#  arguments.


check_o "(\"$KLISP\" \"-e\" \"(write(get-interpreter-arguments))\")" \
    $KLISP -e '(write(get-interpreter-arguments))'
check_o "(\"$KLISP\" \"-e\" \"(write(get-interpreter-arguments))\" \"--\")" \
    $KLISP -e '(write(get-interpreter-arguments))' --
check_oi "(\"$KLISP\" \"-e\" \"(write(get-interpreter-arguments))\" \"-\")" '' \
    $KLISP -e '(write(get-interpreter-arguments))' -
check_o "(\"$KLISP\" \"-e\" \"(write(get-interpreter-arguments))\" \"/dev/null\")" \
    $KLISP -e '(write(get-interpreter-arguments))' /dev/null
check_o "(\"$KLISP\" \"-e(write(get-interpreter-arguments))\" \"--\" \"/dev/null\")" \
    $KLISP '-e(write(get-interpreter-arguments))' -- /dev/null
check_o "(\"$KLISP\" \"-e(write(get-interpreter-arguments))\" \"--\" \"/dev/null\" \"a\" \"b\" \"c\")" \
    $KLISP '-e(write(get-interpreter-arguments))' -- /dev/null a b c

# stdout and stderr

check_oe 'abc' '' $KLISP -e '(display "abc" (get-current-output-port))'
check_oe '' 'abc' $KLISP -e '(display "abc" (get-current-error-port))'

# done

report
cleanup
