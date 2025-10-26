function echo(s) {
    return s;
}

function greet(name) {
    return `Hello, ${name}!`;
}

globalThis.echo  = echo;
globalThis.greet = greet;
