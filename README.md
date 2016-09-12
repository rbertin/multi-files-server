### This server was just an experiment and isn't be destinated to have futures improvements.

Its main use is to be able to delivers multiples files in one time (only one request).
At the time, even is the server is single thread, jobs are splitted by little chunk for to be able to handle many connection (using select(2)).

### Features
- Able to handle Multi client 
- Only two request:
     * GET: for getting only one file
     * LISTFILES: for listing file list before downloading them
- Able to delivers many files in one request 

### TCP request - usage 
	Getting a list of files: `GET /file1;file2` 
	Listing a directory: `LISTFILES /directory/path`
	
	
	"# multi-files-server" 
