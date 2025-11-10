// פונקציה לבדיקת bool: הופכת אמת/שקר
function invert_bool(b) {
    return !Boolean(b);
}
// globalThis.invert_bool = invert_bool;

// אופציונלי: גם AND לוגי
function and_bool(a, b) {
    return Boolean(a) && Boolean(b);
}
// globalThis.and_bool = and_bool;
