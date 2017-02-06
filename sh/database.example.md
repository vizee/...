# myapp

## tables
### users
authentication
```
id          int             // uid
email       varchar(128)    // email
password    char(32)        // md5(salt+password)
type        tinyint         // user type
last_login  int             // last login
create_time int             // create time
update_time int             // update time
```

### users_info
user information
```
id          int             // index
uid         int             // binding uid
nickname    varchar(14)     // nickname
avatar      varchar(128)    // avatar uri
create_time int             // create time
update_time int             // update time
```

### posts
```
id          int             // pid
uid         int             // sender
content     varchar(1024)   // content
create_time int             // create time
update_time int             // update time
```

### votes
```
id          int             // vid
pid         int             // for post id
type        int             // up/down vote
create_time int             // create time
update_time int             // update time
```
