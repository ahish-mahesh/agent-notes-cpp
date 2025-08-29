# TO-DO list for Agent Notes

## Web-Socket Server

- [ ]Implement a simple web-socket API in C++ and test it using postman

## Transcribe

- [x] Save the transcribed audio to DB
- [x] Try out the llama-server instead of directly calling the cpp methods.
      - This is to check if llama has already been optimized as part of the **server**, instead of me having to do it manually.
      - Post note: This approach seems simpler. Decided to go ahead with this.

## Chat

- [ ] Add chat functionality.

## CLI

- [x] Implement a basic CLI
  - [ ] CRUD operations of transcripts
  - [ ] Chat functionality with transcripts

## DB

- [x] Add SQLLite to CMakeLists
- [ ] Setup a SQL-Lite DB to store the transcripts and chats.
  - [x] Transcripts support
  - [ ] Chat support
- [x] Write a helper to interact with the SQL-Lite DB
