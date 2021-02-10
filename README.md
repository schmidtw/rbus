Rbus
----
Unified RDK bus is an initiative to develop a common IPC bus that can be used across all profiles of RDK.
The API spec for this layer is expected to be an replacement of CCSP Common Library IPC framework used by RDK-B.
The APIs are a enhancement of rbus-core APIs which is an intermediate layer that offers RPC and eventing capabilities.

Compilation Steps
-----------------
1. The prerequisite for rbus is to have rbuscore; Make sure that rbuscore compiled and installed. Ref: https://code.rdkcentral.com/r/plugins/gitiles/components/opensource/rbuscore/ for instruction.

2. Create a workspace

   $ mkdir -p ~/workspace
   $ cd  ~/workspace

3. Export the prefix variable. (rbuscore binaries, libraries and headers should have been installed in this path)

   $ export PREFIX=${PWD}
   
4. Create a src folder where this rbus going to be downloaded

   $ mkdir -p src
   $ cd src/

5. Clone the source

   $ git clone https://code.rdkcentral.com/r/components/opensource/rbus


6. Execute the below commands

   $ cd rbus
   $ mkdir build
   $ cmake -DBUILD_FOR_DESKTOP=ON -DCMAKE_INSTALL_PREFIX=$PREFIX ../
   $ make
   $ make install

7. To enable GTest case include the cmake flag '-DENABLE_UNIT_TESTING=ON' and run the make commands
8. To enable code coverage include the cmake flag '-DENABLE_CODE_COVERAGE=ON' and run the make commands

