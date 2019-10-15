---
title: Configure YSQL client connections
linkTitle: Configure YSQL client connections
description: Configure YSQL client connections
headcontent: How to configure fine-grained access control for YSQL clients
image: /images/section_icons/secure/authentication.png
menu:
  latest:
    identifier: ysql-client-authentication
    parent: authentication
    weight: 20
draft: true
isTocNested: true
showAsideToc: true
---

YugabyteDB client authentication for YSQL is managed by two settings c

ontrolled by the host-based configuration file (`yb_hba.conf`), which contains records that specify allowed connection types, users, client IP addresses, and the authentication method. Unlike PostgreSQL, in YugabyteDB you manage the contents of this file using the configuration flag `--ysql_hba_conf`

Before you can managed remote client authentication
The default setting for `listen_addresses, YugabyteDB accepts connections only from `localhost`, To allow remote connections, you must add client authentication records to the host-based authentication configuration file (`yb_hba.conf`), located in the YugabyteDB data directory (`yugabyte-data`).

When a connection request is received by YugabyteDB, the following steps occur:

1. A connection request is received
2. YugabyteDB searches through the `yb_hba.conf` records serially until the first record with a matching connection type, client address, requested database, and username is found.
3. Authentication is performed based on the matching record.
4. If the information provided in the connection request matches the expected content, access is allowed. If authentication fails, then subsequent records are not evaluated and access is denied.

{{< note title="Note" >}}

## For local clusters

For local clusters created using the `yb-ctl` utility, remote client connections are not permitted. The default `listen_addresses='127.0.0.1'` cannot be changed to specify any remote clients.

To configure localhost client connections:

1. Start your YugabyteDB cluster using the `--ysql_hba_conf` option set with any 

="host all yugabyte 0.0.0.0/0 trust,
                 host all all 0.0.0.0/0 md5,
                 host all yugabyte ::0/0 trust,
                 host all all ::0/0 md5"

`--ysql_enable_auth` configuration option during ???

## For deployable clusters

YugabyteDB clusters that are manually deployed can allow remote client authentication by changing the `listen_addresses` to  and the 

To ???

Records in YugabyteDB's `yb_hba.conf` are auto-generated based on the values included in the `--ysql_hba_conf` option. For example, starting a YB-TServer with the following `--ysql_hba_conf` command line option will enable authorization for all users except `yugabyte`:

```
--ysql_hba_conf="host all yugabyte 0.0.0.0/0 trust, host all all 0.0.0.0/0 md5, host all yugabyte ::0/0 trust, host all all ::0/0 md5"
```

OR (easier to read), you can add the following to your `tserver.conf` file and start your 

```
--ysql_hba_conf="host all yugabyte 0.0.0.0/0 trust,
                 host all all 0.0.0.0/0 md5,
                 host all yugabyte ::0/0 trust,
                 host all all ::0/0 md5"
```

{{< /note >}}

You can display the current settings for the `yb_hba.conf` file by getting the file location using the SHOW statement:

```
yugabyte=# SHOW hba_file;
                      hba_file
-------------------------------------------------------
 /Users/yugabyte/yugabyte-data/node-1/disk-1/pg_data/ysql_hba.conf
(1 row)
```

and then

```
yugabyte=# sho hba
# This is an autogenerated file, do not edit manually!
host all all 0.0.0.0/0 trust
host all all ::0/0 trust
/Users/yugabyte/yugabyte-data/node-1/disk-1/pg_data/ysql_hba.conf (END)



The following statement will display the location of the yb_hba.conf`
SHOW hba_file

```
# This is an autogenerated file, do not edit manually!
host all all 0.0.0.0/0 trust
host all all ::0/0 trust
/Users/yugabyte/yugabyte-data/node-1/disk-1/pg_data/ysql_hba.conf (END)
```


## Record formats

Each record in the `yb_hba.conf` configuration file must match one of the seven record formats available for local, IP addresses, or CIDR addresses.

### local

By default, YugabyteDB accepts connections only from `localhost`, where YugabyteDB is running. A "local" record does not need an IP address or netmask to be specified because the assumption is that the connection is from the same server.

```
local database user auth-method [auth-option]
```

### CIDR addresses

CIDR (classless inter-domain routing)

For `host` records with SSL encryption or or non-SSL connection attempts:

```
host database user CIDR-address auth-method  [auth-option]
```

For `host` records with SSL encryption:

```
hostssl database user CIDR-address auth-method  [auth-option]
```

For `host` record from non-SSL connection attempts:

```
hostnossl database user CIDR-address auth-method [auth-option]
```

### IP addresses

```
host database user IP-address netmask auth-method [auth-option]
```

```
hostssl database user IP-address netmask auth-method [auth-option]
```

```
hostnossl database user IP-address netmask auth-method  [auth-option]
```

### Fields

#### local

#### host

#### hostssl

Specify a local or remote host that can connect to YugaByteDB cluster using SSL.

#### hostnossl

#### *database*

#### *user*

#### *address*

#### *IP-address* 

#### *netmask*

Specify the netmask, or subnet mask, that defines the host part of

Applies to host, hostssl, and hostnossl records.

When there is only one host, the netmask is `255.255.255.255`, representing a single IP address. For more information, see [Netmask Quick Reference](http://www.unixwiz.net/techtips/netmask-ref.html).

#### *auth-method*

Specify the authentication method the server should use for a user trying to connect to the server.

##### trust

Specify that any user from the defined host can connect to a YugabyteDB database without requiring a password. If the specified host is not secure or provides access to unknown users, this is a security risk.

##### reject

Specify that the host or user should be rejected.

##### password

Specify that for a connecting user, the password supplied must match the password in the global `yb_show` system table for the username. The password must be sent in clear text.

##### crypt

##### krb4 | krb5

Specify a ???

##### ident

#### *auth-options*

## Add a record

- One record per line.
- Blank lines are ignored.
- Text after the comment character (`#`) is ignored.
- Each record is composed of fields, separated by spaces or tabs.
- Fields can contain white space if the field value is double-quoted.



## Examples

### Single host entry

The following record allows a single host with the IP address `192.168.1.10` to connect to any database (`all`) as any user (`all`) without a password (`trust`).

```text
host all 192.168.1.10 255.255.255.255 trust
```

### Check user permissions

```
testdb=#
SELECT relname as "Relation", relacl as "Access permissions"

testdb-#
       FROM pg_class

testdb-#
       WHERE  relkind IN ('r', 'v', 'S')

testdb-#
       AND relname !~ '^pg_'

testdb-#
       ORDER BY relname;

 Relation |     Access permissions
----------+----------------------------------
 foo      | {"=arwR","jdrake=arwR"}
 my_list  | {"=","jdrake=arwR","jworsley=r"}
(2 rows)
```

------

Start yb-tserver with:

```
--ysql_hba_conf="host all yugabyte 0.0.0.0/0 trust,host all all 0.0.0.0/0 md5,host all yugabyte ::0/0 trust,host all all ::0/0 md5"
```

This will enable authentication for all users except `yugabyte`.