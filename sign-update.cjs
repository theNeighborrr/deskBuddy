// node sign-update.cjs firmware.bin VERSION private-key.pem output.buddy
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const [imageFile,version,keyFile,output]=process.argv.slice(2);
if(!imageFile||!keyFile||!output||!/^\d+\.\d+\.\d+(?:-[a-z0-9]+)?$/.test(version||'')||version.length>15)throw Error('Usage: node sign-update.cjs firmware.bin VERSION private-key.pem output.buddy');
const image=fs.readFileSync(imageFile);if(image.length<1024||image.length>0x1e0000||image[0]!==0xe9)throw Error('Image does not fit the expected ESP32 target');
const key=crypto.createPrivateKey(fs.readFileSync(keyFile));
const publicPem=fs.readFileSync(path.join(__dirname,'DeskBuddy/ReleasePublicKey.h'),'utf8').match(/-----BEGIN PUBLIC KEY-----[\s\S]*?-----END PUBLIC KEY-----/)[0];
const expected=crypto.createPublicKey(publicPem).export({type:'spki',format:'der'}),actual=crypto.createPublicKey(key).export({type:'spki',format:'der'});
if(!expected.equals(actual))throw Error('Private key does not match the embedded release public key');
const header=Buffer.alloc(160);header.write('DBFW030',0,'ascii');header.write('NM-TV-154',8,'ascii');header.writeUInt32LE(image.length,24);header.write(version,28,'ascii');crypto.createHash('sha256').update(image).digest().copy(header,44);
const signature=crypto.sign('sha256',header.subarray(0,76),key);if(signature.length>72)throw Error('Unexpected signature size');header.writeUInt16LE(signature.length,76);signature.copy(header,78);
if(!crypto.verify('sha256',header.subarray(0,76),publicPem,signature))throw Error('Signature verification failed');
fs.writeFileSync(output,Buffer.concat([header,image]));console.log('Signed update written:',output);
