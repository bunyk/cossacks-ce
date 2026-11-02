# Cossacks CE Porting

IMPORTANT: Do NOT run the produced executable directly!
The game is interactive and will hang. Ask user to run and provide logs

Try to minimize code changes, especially movements and try instead moving

```
#ifdef _WIN32
...
#endif // _WIN32
```

blocks. Or get rid of them if code verified to be cross-platform.
We aim for short verifiable changes.

When investigating code, feel free to leave comments about the code purpose,
meaning of function parameters, possible values for enums, etc.
So code is more readable than before, and requires less investigation later.

When in need to figure out how something happens we could use plain old printf to STDOUT.

Once done, run `echo -ne '\007'` to notify me.
