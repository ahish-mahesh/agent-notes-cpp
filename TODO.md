# TO-DO list for Agent Notes

## Web-Socket Server

- [ ] Implement a simple web-socket API in C++ and test it using postman

## Transcribe

- [x] Save the transcribed audio to DB
- [ ] Try out the llama-server instead of directly calling the cpp methods.
      - This is to check if llama has already been optimized as part of the server, instead of me having to do it manually.

## DB

- [x] Add SQLLite to CMakeLists
- [ ] Setup a SQL-Lite DB to store the transcripts and chats.
  - [x] Transcripts support
  - [ ] Chat support
- [x] Write a helper to interact with the SQL-Lite DB
