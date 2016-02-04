# MSGPACK protocol via redis between wyliodrin-server and wyliodrin-hypervisor.

Every mesage transmitetted between wyliodrin-server and wyliodrin-hypervisor is a msgpack map. Every value is a string. All maps must have the "a" key ("a" stands for "action"). The value of this key must be one of the
following:
  * o (open)
  * c (close)
  * k (keys)
  * s (status)
  * p (poweroff)
  * d (disconnect)
  * r (resize)



##open
```
-->
{
  "a": "o",    // "action":    "open" (required)
  "r": "19",   // "request":   "19"   (required)
  "w": "80",   // "width":     "80"   (required)
  "h": "20",   // "height":    "20"   (required)
  "p": "1234", // "projectid": "1234" (optional: only for running projects)
  "u": "5678"  // "userid":    "5678" (optional: only for running projects)
}
```

```
<--
{
  "a" : "o",          // "action":   "open"          (required)
  "r" : "19",         // "request":  "19"            (required)
  "re": "done/error", // "response": "done or error" (required)
  "s" : "3",          // "shellid":  "3"             (optional: only for respone = done)
  "ru": "true/false"  // "running":  "true/false"    (optional: only for respone = done)
}
```



##close
```
-->
{
  "a": "c",    // "action":  "close" (required)
  "r": "19",   // "request": "19"    (required)
  "s" : "3",   // "shellid": "3"     (required)
}
```

```
<--
{
  "a": "c",    // "action":       "close" (required)
  "r": "19",   // "request":      "19"    (required)
  "s": "3",    // "shellid":      "3"     (required)
  "c": "0",    // "code of exit": "0"     (required)
}
```


##keys
```
-->
{
  "a" : "k",         // "action":  "keys"       (required)
  "r" : "19",        // "request": "19"         (required)
  "s" : "3",         // "shellid": "3"          (required)
  "t" : "plain text" // "text":    "plain text" (required)
}
```

```
<--
{
  "a" : "k"          // "action":  "keys"       (required)
  "r" : "19"         // "request": "19"         (required)
  "s" : "3"          // "shellid": "3"          (required)
  "t" : "plain text" // "text":    "plain text" (required)
}
```



##status
```
-->
{
  "a": "s",   // "action":    "status" (required)
  "r": "19",  // "request":   "19"     (required)
  "p": "1234" // "projectid": "1234"   (required)
}
```

```
<--
{
  "a":  "s",         // "action":    "status"     (required)
  "r":  "19",        // "request":   "19"         (required)
  "p":  "1234",      // "projectid": "1234"       (required)
  "ru": "true/false" // "running":   "true/false" (required)
}
```



## poweroff
```
-->
{
  "a": "p" // "action": "poweroff" (required)
}
```



## disconnect
```
-->
{
  "a": "d",  // "action":  "disconnect" (required)
  "r": "19", // "request": "19"         (required)
  "s": "3"   // "shellid": "3"          (required)
}
```



##resize
```
-->
{
  "a": "r",  // "action":  "resize" (required)
  "r": "19", // "request": "19"     (required)
  "s": "3"   // "shellid": "3"      (required)
  "w": "80", // "width":   "80"     (required)
  "h": "20", // "height":  "20"     (required)
}
```
