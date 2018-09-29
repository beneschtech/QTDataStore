# QTDataStore
A fast file based object oriented data store that uses NoSQL, hashing storage tree, and DBus to store and retrieve documents

Required libraries:
C++11
QT5.7+ (Core, DBus, qmake)
lmdb-devel
openssl-devel (for SHA256)

This is meant to be an application library with some examples provided to show usage.  If you need to store a large number of files that you are doing frequent writes and reads from both local and remote sources.  Borrows ideas from Squid's storage engine, uses LMDB as a backend to store metadata in JSON (QVariant) format, and uses Qt to both abstract the object metadata, and provide the dbus interface in an extendable, easy to understand way.  The one major limitation, but one we deal with for speeds sake is that since it is NoSQL, we can only use one column as an index.

The motivation for writing this was due to a problem we had at work in several places and needed a similar solution for all of them.  Basically we have many processes both local and remote that create aand read files from a "Database" that is a time indexed directory structure (YYYY/MM/DD/HH) with an index file that lists the files known and their metadata.  Worker threads go through and update those files based on hidden files that are "new".  The problem was these files could be coming not only from another process, but another system and only be half written causing the reader to crash, and leave the directory in an unusable state, plus everything was communicating over legacy RPC and later a self hand rolled communication system involving JSON and TCP sockets that wasnt entirely stable (or thread safe).

It was a real mess and a real maintenance headache.  We tossed the idea of using NoSQL to store the files known around, but one major drawback with any NoSQL system.  Although we could have many readers/writers on one host, in our setup that wouldnt work with multiple hosts all reading/writing to the same store.  So now this is the culmination of that with D-Bus (which can be ran TCP aware) as a communication mechanism and local reads/writes will use the database calls, and remote ones will use DBus to inform of new files written.  

In addition to that since the database is storing all of the files and their metadata, we dont need to put the files in human useful locations.  Another issue that we had in the above mentioned directory structure is that sometimes some of the directories could grow to thousands or even tens of thousands of files leading to slow reads/writes. So we will take the metadata and SHA256 it (to remain FIPS compliant) and use the first 4 hex numbers to generate a 256/256 directory structure with the rest of the hash under it as a filename.  This should provide a fairly balanced tree and allow for quick read/writes no matter how much is thrown at it.  This will leave administration and troublehsooting to using the database as a reference and looking for files not in there or vice versa.  Also the hashing mechanism replaces the unreliable mkstemp call over NFS filesystems. Mkstemp, although useful in practice on a local host is begging for lost data and name collissions on an NFS mount.

The basic idea of what we are trying to achieve and run fast and reliably is one server with 20-30 processes all writing data to the store as new raw data, then 3-4 hosts with 40-50 processes apiece on an NFS mount reading said data, processing it, then writing the completed data to the data store, among other things.

This is a QT project, and needs a QT development system installed.  Please look at the file QTDataStore.libs and modify as necessary for your OS.  Eg. LMDBLIBS="-L/usr/local/lib -llmdb" 
