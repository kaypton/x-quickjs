

function entry() {
    
    binderjs.import("@service_print/print.js");
    var ret = binderjs.call("print", "print", "hello,from main 1")
    console.log("ret:" + ret);
    binderjs.call("print", "print2", "sen1", "sen2")
    binderjs.release("print");

    binderjs.import("@service_add/add.js");
    ret = binderjs.call("add", "add", 1, 2)
    console.log("add ret:" + ret);
    binderjs.release("add");
}


