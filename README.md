# webserver
a webserver developed for BS course assignment

##### Usage

default port for web server is 8000, for php support, install php-fpm first.

For macOS:

```bash
brew install php-fpm
```

copy the default `php.ini` and `php-fpm.conf`

```bash
sudo cp /etc/php.ini.default /etc/php.ini
chown <user> /etc/php.ini
chmod u+w /etc/php.ini
sudo cp /private/etc/php-fpm.conf.default /private/etc/php-fpm.conf
sudo chmod 777 /private/etc/php-fpm.conf
```

change the default error log and the listen port

```bash
vi /private/etc/php-fpm.conf
```

Change the `error_log` to places exist, and change the port

```bash
listen = 127.0.0.1:9000
```

may also need to change the user from nobody to current user to get access to the file

```
vi /etc/php-fpm.d
```

or may chmod 777 the file we want to read

start the php-fpm

```bash
sudo php-fpm
```

since we don't have service in macOS, when killing the service, we may have to use killall

```bash
sudo killall php-fpm
```