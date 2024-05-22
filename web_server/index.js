import { WebSocketServer } from 'ws';
import http from 'http';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

const port = 22800;

const server = http.createServer((request, response) => {
    if (request.url === '/') {
        const file_path = path.join(__dirname, 'index.html');
        fs.readFile(file_path, (err, data) => {
            if (err) {
                response.writeHead(404);
                response.end('Not found');
            } else {
                response.writeHead(200, { 'Content-Type': 'text/html' });
                response.end(data);
            }
        });
    } else if (request.url === '/list-data-files') {
        const directory_path = path.join(__dirname, 'data');
        fs.readdir(directory_path, (err, files) => {
            if (err) {
                response.writeHead(500);
                response.end('Server Error');
                return;
            }
            response.writeHead(200, { 'Content-Type': 'application/json' });
            response.end(JSON.stringify(files.filter(file => file.endsWith('.png'))));
        });
    } else {
        let file_path = path.join(__dirname, request.url);
        const content_type = {
            '.html': 'text/html',
            '.png': 'image/png'
        }[path.extname(file_path)];

        if (content_type) {
            fs.readFile(file_path, (err, data) => {
                if (err) {
                    response.writeHead(404);
                    response.end('Not found');
                } else {
                    response.writeHead(200, { 'Content-Type': content_type });
                    response.end(data);
                }
            });
        } else {
            response.writeHead(404);
            response.end('Not found');
        }
    }
});

const web_socket_server = new WebSocketServer({ server });

web_socket_server.on('connection', (web_socket, request) => {
    const client_address = request.socket.remoteAddress.replace('::ffff:', '');
    console.info(`${client_address} connected`);

    web_socket.on('message', (message) => {
        web_socket_server.clients.forEach(client => client.send(message));
    });

    web_socket.on('close', () => console.info(`${client_address} disconnected \n`));
    web_socket.on('error', error => console.error(error));
});

server.listen(port, () => {
    console.log(`[+] server is running on http://localhost:${port}`);
});
