set -e

rm -rf /home/testuser/testshare
mkdir /home/testuser/testshare
mkdir /home/testuser/testshare/testdir
echo "this is a test file" > /home/testuser/testshare/testdir/testfile.txt
chown -R testuser /home/testuser
mkdir /home/testuser/testshare/noaccess
chmod 0700 /home/testuser/testshare/noaccess

# Our test user can write to /home/testuser/testshare:
chmod 0777 /home/testuser/testshare
