/**
 * Format an SVG file exported from Illustrator in the inkscape-style format that VCVRack wants
 */

const fs = require('fs');
const path = require('path');
const { XMLParser, XMLBuilder, XMLValidator} = require("fast-xml-parser");
const { SVGPathData, encodeSVGPath } = require("svg-pathdata");
const panelDir = path.join(__dirname, '../panels');
const SCALE = 75 / 72;

const dir = fs.opendirSync(panelDir);

// Iterate through the directory, reading each subdirectory and processing each .in.svg file

// This is the synchronous version, which is easier to read
let dirent;
while ((dirent = dir.readSync()) !== null) {
    if (dirent.isDirectory()) {
        fs.readdirSync(path.join(panelDir, dirent.name)).forEach(file => {
            if (!file.endsWith('.in.svg')) {
                return;
            }

            console.log(`Processing ${file}`);
            const fullpath = path.join(panelDir, dirent.name, file);
            formatSVG(fullpath);
        });
    }
}

function scaleValue(val) {
    const num = parseFloat(val);
    return isNaN(num) ? val : (num * SCALE).toFixed(2);
}

// Scale attributes in a tag
function scaleAttributes(tag, keys) {
    if (!tag[':@']) return;
    for (const key of keys) {
        if (key in tag[':@']) {
            tag[':@'][key] = scaleValue(tag[':@'][key]);
        }
    }
}

function scalePathData(d, scale) {
    const pathData = new SVGPathData(d);
    const transformed = pathData.toAbs().scale(scale, scale);
    return encodeSVGPath(transformed.commands);
}

// Scale path data (only absolute commands for now)
// function scalePathData(d) {
//     // Very naive parser, assumes space/comma separated absolute commands
//     return d.replace(/([MmLlHhVvCcSsQqTtAaZz])([^MmLlHhVvCcSsQqTtAaZz]*)/g, (match, cmd, params) => {
//         if (/z/i.test(cmd)) return cmd; // no coordinates
//         const scaledParams = params
//             .trim()
//             .split(/[\s,]+/)
//             .map(scaleValue)
//             .join(' ');
//         return `${cmd}${scaledParams}`;
//     });
// }

// function scalePathData(d) {
//     return d.replace(/([+-]?\d*\.?\d+(?:e[+-]?\d+)?)/gi, num => {
//         return scaleValue(parseFloat(num));
//     });
// }

// function scalePathData(d) {
//     const tokens = [];
//     let current = '';
//     let isNumberChar = c => /[0-9eE.\-+]/.test(c);

//     for (let i = 0; i < d.length; i++) {
//         const char = d[i];

//         if (isNumberChar(char)) {
//             current += char;
//         } else {
//             if (current.length > 0) {
//                 tokens.push(current);
//                 current = '';
//             }
//             if (char.trim() !== '') {
//                 tokens.push(char);
//             } else if (char === ' ') {
//                 tokens.push(' ');
//             }
//         }
//     }

//     if (current.length > 0) {
//         tokens.push(current);
//     }

//     // Scale numbers only
//     const scaledTokens = tokens.map(tok => {
//         if (!isNaN(tok) && tok.trim() !== '') {
//             return scaleValue(parseFloat(tok)).toString();
//         }
//         return tok;
//     });

//     // Reassemble, preserving commas and spaces
//     return scaledTokens.join('').replace(/ ?([A-Za-z]) ?/g, '$1');
// }

// function scalePathData(d) {
//     const commandPattern = /[a-zA-Z]/;
//     const numberPattern = /[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?/g;

//     let result = '';
//     let index = 0;

//     while (index < d.length) {
//         const char = d[index];

//         if (commandPattern.test(char)) {
//             result += char;
//             index++;
//         } else if (char === ',' || char === ' ') {
//             result += char;
//             index++;
//         } else {
//             // It's the start of a number
//             numberPattern.lastIndex = index;
//             const match = numberPattern.exec(d);
//             if (match && match.index === index) {
//                 const number = parseFloat(match[0]);
//                 const scaled = scaleValue(number);
//                 result += scaled;
//                 index += match[0].length;
//             } else {
//                 // Unrecognized character (fail-safe)
//                 result += d[index];
//                 index++;
//             }
//         }
//     }

//     return result;
// }

function walk(obj, fn) {
    if (Array.isArray(obj)) {
        obj.forEach(item => {
            walk(item, fn);
        });
    } else if (typeof obj === 'object' && obj !== null) {
        fn(obj);

        for (const key in obj) {
            if (key === 'parent') continue; // skip parent property
            walk(obj[key], fn);
        }
    }
}

function formatSVG(file) {
    const data = fs.readFileSync(file, 'utf8');
    const parserOptions = {
        ignoreAttributes : false,
        preserveOrder: true
    };
    const parser = new XMLParser(parserOptions);
    let jObj = parser.parse(data);

    const svgelt = jObj[1];

    // Scale elements to 75 dpi
    // Traverse all elements
    walk(svgelt, node => {

        // Scale known element types
        if (node.linearGradient) {
            scaleAttributes(node, ['@_x1', '@_y1', '@_x2', '@_y2', '@_cx', '@_cy', '@_r', '@_fx', '@_fy']);
        }

        if (node.rect) {
            scaleAttributes(node, ['@_x', '@_y', '@_width', '@_height', '@_rx', '@_ry']);
        }

        if (node.line) {
            scaleAttributes(node, ['@_x1', '@_y1', '@_x2', '@_y2']);
        }

        if (node.circle) {
            scaleAttributes(node, ['@_cx', '@_cy', '@_r']);
        }

        if (node.path) {
            if (node[':@'] && node[':@']['@_d']) {
                node[':@']['@_d'] = scalePathData(node[':@']['@_d'], SCALE);
            }
        }
    });
    
    const viewBox = svgelt[':@']['@_viewBox'].split(' ');

    // Scale all viewBox components
    const scaledViewBox = viewBox.map(scaleValue);
    svgelt[':@']['@_viewBox'] = scaledViewBox.join(' ');

    // Add a height and width attribute to the root svg element, using the viewBox attribute
    svgelt[':@']['@_height'] = scaledViewBox[3];
    svgelt[':@']['@_width'] = scaledViewBox[2];

    // For elements in the group with the id "components", modify the "style" attribute
    // Remove whitespace after the colon, and change color names to hex values
    const components = svgelt.svg.find(g => !!g[':@'] && g[':@']['@_id'] === 'components');

    if (!!components) {
        components['g'].forEach(g => {
            if (g[':@'] && g[':@']['@_style']) {
                g[':@']['@_style'] = g[':@']['@_style']
                    .replace(/:\s+/g, ':')
                    .replace(/red/g, '#FF0000')
                    .replace(/green/g, '#00FF00')
                    .replace(/blue/g, '#0000FF')
                    .replace(/lime/g, '#00FF00')
                    .replace(/#f0f/g, '#FF00FF')
            }
        });

        // Add display="none" to the group with the id "components"
        components[':@']['@_display'] = 'none';
    }
    
    const builderOptions = {
        ignoreAttributes : false,
        format: true,
        preserveOrder: true
    };
    const builder = new XMLBuilder(builderOptions);
    const xmlContent = builder.build(jObj);

    // write to an output, replacing the .in.svg suffix with .svg
    const out = file.replace('.in.svg', '.svg');
    fs.writeFileSync(out, xmlContent);
}
