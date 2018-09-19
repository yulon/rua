#!/usr/bin/env node

const fs = require("fs")
const cp = require("child_process")
const path = require("path")

var log
var argv = process.argv.splice(2);
if (argv.length > 0 && argv[0] === "-v") {
	log = console.log
} else {
	log = () => {}
}

function mkdir(dir) {
	if (fs.existsSync(dir)) {
		return
	}
	mkdir(path.dirname(dir))
	fs.mkdirSync(dir)
}

const repoDir = path.dirname(path.dirname(__dirname))
const binDir = path.join(repoDir, "build", "cont_bin")
const incDir = path.join(repoDir, "include", "rua", "cont")

mkdir(binDir)
mkdir(incDir)

const files = fs.readdirSync(__dirname)

files.forEach(srcPath => {
	if (path.extname(srcPath) !== ".s") {
		return
	}

	log("build: " + srcPath)

	const name = path.basename(srcPath).split(".")[0]

	const binPath = path.join(binDir, name + ".bin")
	const stdout = cp.execFileSync(
		"fasm",
		[path.join(__dirname, srcPath), binPath]
	).toString()

	log(stdout.length ? stdout + "\b" : "")

	const data = fs.readFileSync(binPath)

	var incStr = ""
	var first = true
	for (let i = 0; i < data.length; i++) {
		if (first) {
			first = false
		} else {
			incStr += ", "
		}
		incStr += "0x" + data[i].toString(16).toUpperCase()
	}
	incStr += "\n"

	var incPath = path.join(incDir, name + ".inc")
	fs.writeFileSync(incPath, incStr)

	log("output: " + incPath + "\n")
})
