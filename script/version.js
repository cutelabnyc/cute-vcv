#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const semver = require('semver');
const { execSync } = require('child_process');
const { Command } = require('commander');

const program = new Command();

program
  .argument('[project]', 'project/directory name (example: vcv-rack, )', 'vcv-rack')
  .argument('[version_json]', 'path to version JSON file (default: version.json in project directory)', 'version.json')
  .argument('[bump]', 'version bump type (major, minor, patch, prerelease, prepatch, etc.)', 'patch')
  .option('--version <version>', 'set the version number instead of bumping it')
  .option('--dry-run', 'show what would happen without writing files or running git commands')
  .parse();

const options = program.opts();
const packageDir = program.args[0];
const versionFile = program.args[1];

if (program.args.length < 2 && !options.version)  {
  console.error('❌ Provide either the bump type as an argument or use --version to set a specific version.');
  program.help();
  process.exit(1);
}

const bumpType = options.version ? 'set' : program.args[2];
const versionJsonPath = path.join(packageDir, versionFile);

function runCmd(cmd) {
  if (options.dryRun) {
    console.log(`[dry-run] ${cmd}`);
  } else {
    execSync(cmd, { stdio: 'inherit' });
  }
}

try {
  if (!fs.existsSync(versionJsonPath)) {
    console.error(`❌ Could not find version file at ${versionJsonPath}`);
    process.exit(1);
  }

  const versionDataRaw = fs.readFileSync(versionJsonPath, 'utf8');
  const versionData = JSON.parse(versionDataRaw);
  const currentVersion = versionData.version;

  if (!semver.valid(currentVersion)) {
    console.error(`❌ Invalid semver version in ${versionJsonPath}: ${currentVersion}`);
    process.exit(1);
  }

  let newVersion = currentVersion;

  if (bumpType !== 'set') {
    newVersion = semver.inc(currentVersion, bumpType);
    if (!newVersion) {
      console.error(`❌ Failed to bump version from ${currentVersion} using bump type '${bumpType}'`);
      process.exit(1);
    }

    console.log(`Version bump: ${currentVersion} → ${newVersion}`);
  } else {
    if (!options.version || !semver.valid(options.version)) {
      console.error(`❌ Invalid version specified: ${options.version}`);
      process.exit(1);
    }
    newVersion = options.version;

    console.log(`Setting version to: ${newVersion}`);
  }

  if (!options.dryRun) {
    versionData.version = newVersion;
    fs.writeFileSync(versionJsonPath, JSON.stringify(versionData, null, 2) + '\n');
  } else {
    console.log(`[dry-run] Would update ${versionJsonPath} with new version`);
  }

  const tagName = `${packageDir}/v${newVersion}`;

  runCmd(`git add ${versionJsonPath}`);
  runCmd(`git commit -m "${packageDir}: bump version to ${newVersion}"`);
  runCmd(`git tag ${tagName}`);

  console.log(`✅ ${options.dryRun ? '[dry-run] ' : ''}Bumped ${packageDir} to ${newVersion} and tagged as ${tagName}`);

  if (!options.dryRun) {
    console.log(`Created git tag: ${tagName}`);
    console.log(`Please review tag name and commit before pushing.`);
  }

} catch (err) {
  console.error(`❌ Error: ${err.message}`);
  process.exit(1);
}
