const fs = require('fs');

const numBleps = 16;
const blepDuration = 2.0;
const tableSize = 512;
const sampleRate = 48000;
const blepMinFreq = 40;

// sinc params
const bandwidth = 11;

function hammingWindow(i, size) {
    return 0.54 - 0.46 * Math.cos(2 * Math.PI * i / tableSize);
}

function fftComplex(real, imag) {
    const n = real.length;
    if ((n & (n - 1)) !== 0) throw new Error("FFT size must be a power of 2");
    
    // Bit-reverse permutation
    for (let i = 1, j = 0; i < n; i++) {
        let bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        
        if (i < j) {
            [real[i], real[j]] = [real[j], real[i]];
            [imag[i], imag[j]] = [imag[j], imag[i]];
        }
    }
    
    // Cooley-Tukey decimation-in-time radix-2 FFT
    for (let len = 2; len <= n; len <<= 1) {
        const angle = 2 * Math.PI / len;
        const wlen_re = Math.cos(angle);
        const wlen_im = Math.sin(angle);
        
        for (let i = 0; i < n; i += len) {
            let w_re = 1;
            let w_im = 0;
            
            for (let j = 0; j < len / 2; j++) {
                const u_re = real[i + j];
                const u_im = imag[i + j];
                const v_re = real[i + j + len / 2] * w_re - imag[i + j + len / 2] * w_im;
                const v_im = real[i + j + len / 2] * w_im + imag[i + j + len / 2] * w_re;
                
                real[i + j] = u_re + v_re;
                imag[i + j] = u_im + v_im;
                real[i + j + len / 2] = u_re - v_re;
                imag[i + j + len / 2] = u_im - v_im;
                
                const temp_re = w_re * wlen_re - w_im * wlen_im;
                w_im = w_re * wlen_im + w_im * wlen_re;
                w_re = temp_re;
            }
        }
    }
    
    return { real, imag }; // In-place modification
}

function ifftComplex(real, imag) {
    // Conjugate
    for (let i = 0; i < imag.length; i++) {
        imag[i] = -imag[i];
    }
    
    // Forward FFT
    fftComplex(real, imag);
    
    // Conjugate and normalize
    const n = real.length;
    for (let i = 0; i < n; i++) {
        real[i] /= n;
        imag[i] = -imag[i] / n;
    }
    
    return { real, imag };
}

function bandlimitAndDownsample(input, downsampleFactor) {
    const real = new Float32Array(input);
    const n = real.length;
    const imag = new Float32Array(n);
    
    // FFT the input
    fftComplex(real, imag);
    
    // Calculate cutoff - Nyquist frequency after downsampling
    const cutoffBin = Math.floor(n / (2 * downsampleFactor));
    
    // Zero out frequencies above cutoff (maintain conjugate symmetry)
    for (let k = cutoffBin; k < n - cutoffBin; k++){
        real[k] = 0;
        imag[k] = 0;
    }
    
    // IFFT back to time domain
    ifftComplex(real, imag);
    
    // Now simple decimation (no need for fancy filtering)
    const outputLength = Math.floor(n / downsampleFactor);
    const output = new Float32Array(outputLength);
    
    for (let i = 0; i < outputLength; i++) {
        output[i] = real[i * downsampleFactor];
    }
    
    return output;
}

function makeBlepTable(freq, size) {
    let lut = [];
    const cutoffFreq = 0.5 * sampleRate;
    const normalizedFreq = freq / cutoffFreq;

    for (let i = 0; i < size; i++) {
        const t = (i - size / 2) / sampleRate;
        const x = Math.PI * normalizedFreq * t * sampleRate;
        const y = sinc(x) * hammingWindow(i, size);
        lut.push(y);
    }

    let sum = lut.reduce((a, b) => a + b, 0);
    lut = lut.map(x => x / sum);
    return lut;
}

function makeSincTable() {
    let lut = [];
    for (let i = 0; i < tableSize + 1; i++) {
        let phase = (-Math.PI + i * 2 * Math.PI / tableSize) * bandwidth;
        let x = Math.sin(phase); 
        if (phase === 0) {
            x = 1;
        } else {
            x /= phase;
        }
        lut.push(x * 0.98); // headroom
    }

    lut = lut.map(x => Math.round(Math.pow(2, 31) * x));
    return lut;
}

function makeRampTable() {
    let lut = [];
    for (let i = 0; i < tableSize; i++) {
        lut.push(Math.round(Math.pow(2, 31) * i / tableSize));
    }
    return lut;
}

function minBlepImpulse(z, o) {
    // z = zero crossings on each side, o = oversampling factor
    const n = 2 * z * o;
    
    // Create symmetric sinc array
    const x = new Float32Array(n);
    for (let i = 0; i < n; i++) {
        const p = (i / (n - 1)) * (2 * z) - z; // Rescale to [-z, z]
        x[i] = sinc(p);
    }
    
    // Apply Blackman-Harris window
    blackmanHarrisWindow(x);
    
    // Real cepstrum calculation
    const fx = new Float32Array(2 * n);
    
    // Forward FFT
    const real = Array.from(x);
    const imag = new Array(n).fill(0);
    fftComplex(real, imag); // In-place FFT
    
    // Convert to log magnitude spectrum
    fx[0] = Math.log(Math.abs(real[0])); // DC component
    for (let i = 1; i < n; i++) {
        const magnitude = Math.sqrt(real[i] * real[i] + imag[i] * imag[i]);
        fx[2 * i] = Math.log(Math.max(magnitude, Math.exp(-30))); // Clamp to avoid -inf
        fx[2 * i + 1] = 0;
    }
    fx[1] = Math.log(Math.abs(real[n/2] ? real[n/2] : real[1]));
    
    // Inverse FFT to get cepstrum
    const cepstrum_real = new Array(n);
    const cepstrum_imag = new Array(n).fill(0);
    for (let i = 0; i < n; i++) {
        cepstrum_real[i] = i < fx.length/2 ? fx[2*i] : 0;
    }
    ifftComplex(cepstrum_real, cepstrum_imag);
    
    // Minimum-phase reconstruction
    const minPhase = Array.from(cepstrum_real);
    for (let i = 1; i < n/2; i++) {
        minPhase[i] *= 2;
    }
    for (let i = Math.floor((n + 1)/2); i < n; i++) {
        minPhase[i] = 0;
    }
    
    // Forward FFT of modified cepstrum
    const minPhase_imag = new Array(n).fill(0);
    fftComplex(minPhase, minPhase_imag);
    
    // Convert back from log domain
    for (let i = 0; i < minPhase.length; i++) {
        const magnitude = Math.exp(minPhase[i]);
        const phase = minPhase_imag[i];
        minPhase[i] = magnitude * Math.cos(phase);
        minPhase_imag[i] = magnitude * Math.sin(phase);
    }
    
    // Inverse FFT to get minimum-phase impulse
    ifftComplex(minPhase, minPhase_imag);

    // Integration (impulse -> step response)
    let total = 0;
    for (let i = 0; i < n; i++) {
        total += minPhase[i];
        minPhase[i] = total;
    }
    
    // Normalize to [0, 1]
    const norm = 1.0 / minPhase[n - 1];
    for (let i = 0; i < n; i++) {
        minPhase[i] *= norm;
    }
    
    return minPhase;
}

function blackmanHarrisWindow(x) {
    const n = x.length;
    for (let i = 0; i < n; i++) {
        const t = i / (n - 1);
        const window = 0.35875 - 0.48829 * Math.cos(2 * Math.PI * t) + 
                      0.14128 * Math.cos(4 * Math.PI * t) - 0.01168 * Math.cos(6 * Math.PI * t);
        x[i] *= window;
    }
}

function sinc(x) {
    if (Math.abs(x) < 1e-9) return 1.0;
    return Math.sin(Math.PI * x) / (Math.PI * x);
}

function writeTableToFile(lut, filename) {
    const fd = fs.openSync(filename, 'w');
    if (lut instanceof Float32Array) {
        lut = Array.from(lut); // Convert Float32Array to regular array for JSON serialization
    }
    const table = JSON.stringify(lut);
    fs.writeSync(fd, table);
    fs.closeSync(fd);
}

function writeBleps() {
    const blepTable = minBlepImpulse(8, 16);
    writeTableToFile(blepTable, 'minblep.json');
}

function writeSquareLUT() {
    const maxTableSize = 512;
    const minTableSize = 4;
    const oversampleFactor = 16;
    const oversampled = new Float32Array(maxTableSize * oversampleFactor);

    for (let i = 0; i < oversampled.length; i++) {
        const phase = i / oversampled.length;
        oversampled[i] = (phase < 0.5) ? 1 : -1;
    }

    for (let i = 0; ; i++) {
        const size = Math.floor(maxTableSize / Math.pow(2, i));
        if (size < minTableSize) break;

        const downsampled = bandlimitAndDownsample(oversampled, oversampleFactor * Math.pow(2, i));

        const filename = `bandlimited_${size}.json`;
        writeTableToFile(downsampled, filename);
    }
}

writeBleps();
