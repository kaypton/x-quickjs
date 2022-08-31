
function print2(p, p1) {
    console.log("[1]" + p + "[2]" + p1)
}


function print(p) {
    function print2() {
        return "I am spiderman"
    }
    console.log(p);
    return print2();
}



