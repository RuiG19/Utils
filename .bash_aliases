# exports
export GTEST_ROOT=/usr/src/googletest/googletest
export JAVA_HOME=


# aliases
alias ccat='pygmentize -g -O style=friendly'

alias lgrep='ls -alF | grep -i'
alias hgrep='history | grep -i'

alias codegrep='grep -irn --include=\*.cpp --include=\*.c --include=\*.hpp --include=\*.h --exclude-dir=build'

# util functions
lcat()
{
    cat -n $1 | ccat
}    

cgrep()
{
    ccat $1 | grep -in $2
}

run() {
    number=$1
    shift
    for i in `seq $number`; do
        $@
    done
}

rbuild()
{
    BUILD_DIR="build"
    RUN_CLEAN_BUILD=false
    RUN_BUILD=true
    USE_NINJA=false

    while getopts :hcn opt;  do   
        case $opt in     
            h)
                RUN_BUILD=false
                echo "Usage: run_build.sh [options]"
                echo "Options:"
                echo "  -c          Clean build."
                echo "  -n          Use Ninja generator."
                echo "  -h          Display this information."
                ;;
            c)      
                RUN_CLEAN_BUILD=true
                ;;    
            n)
                USE_NINJA=true
                ;;
        esac 
    done 

    if $RUN_BUILD = true ; then
        if [ ! -d "$BUILD_DIR/" ]; then
            RUN_CLEAN_BUILD=true
        fi

        if [ "$RUN_CLEAN_BUILD" = true ] ; then
            rm -rf $BUILD_DIR/

            if [ "$USE_NINJA" = true ] ; then
                cmake -B$BUILD_DIR -GNinja
            else
                cmake -B$BUILD_DIR
            fi
        fi
        
        cmake --build $BUILD_DIR/ -j 16
    fi
}
