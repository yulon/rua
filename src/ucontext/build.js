#!/usr/bin/env node

const fs = require("fs")
const cp = require("child_process")
const path = require("path")

function mkdir(dir) {
	if (fs.existsSync(dir)) {
		return
	}
	mkdir(path.dirname(dir))
	fs.mkdirSync(dir)
}

const repoDir = path.dirname(path.dirname(__dirname))
const binDir = path.join(repoDir, "bin", "ucontext")
const incDir = path.join(repoDir, "include", "rua", "ucontext")

mkdir(binDir)
mkdir(incDir)

const filePaths = process.argv.slice(1)

filePaths.forEach(filePath => {
	if (filePath.slice(filePath.length - 7) === "_fasm.s") {
		fasm(filePath)
		return
	}
	if (filePath.slice(filePath.length - 6) === "_gas.s") {
		gas(filePath)
		return
	}
})

function fasm(srcPath) {
	console.log("=> build: " + srcPath)

	const srcName = path.basename(srcPath)
	const name = srcName.slice(0, srcName.lastIndexOf("_"))
	const binPath = path.join(binDir, name + ".bin")

	const r = cp.execFileSync(
		"fasm",
		[srcPath, binPath]
	).toString()
	console.log(r.length ? r + "\b" : "")

	bin2hex(binPath)
}

function bin2inc(name) {
	const data = fs.readFileSync(path.join(binDir, name + ".bin"))

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
	console.log("=> output: " + incPath + "\n")
}

function gas(srcPath) {
	console.log("=> build: " + srcPath)

	const srcName = path.basename(srcPath)
	const arch = srcName.slice(0, srcName.lastIndexOf("_"))
	const objPath = path.join(binDir, arch + ".o")

	var r = cp.execFileSync(
		"gcc",
		["-c", srcPath, "-o", objPath]
	).toString()
	console.log(r.length ? r + "\b" : "")

	r = cp.execFileSync(
		"objdump",
		["-d", objPath]
	).toString()
	console.log(r.length ? r + "\b" : "")

	od2inc(arch, r)
}

function od2inc(arch, od) {
	var name = ""
	var incStr = ""
	var first = true
	var out = () => {
		incStr += "\n"
		var incPath = path.join(incDir, name + ".inc")
		fs.writeFileSync(incPath, incStr)
		console.log("=> output: " + incPath + "\n")
	}
	const odLns = od.split("\n")
	for (let i = 0; i < odLns.length; i++) {
		const odLn = odLns[i].trim()

		if (odLn.slice(odLn.length - 2) == ">:") {
			if (name.length != 0) {
				out()
				name = ""
				incStr = ""
				first = true
			}
			const ix = odLn.lastIndexOf("<")
			name = odLn.slice(ix + 1, odLn.length - 2) + "_" + arch
			continue
		}

		const codeLn = odLn.split("\t")
		if (codeLn.length < 3) {
			continue
		}

		const hexs = codeLn[1].trim().split(" ")
		for (let j = 0; j < hexs.length; j++) {
			for (let k = hexs[j].length - 2; k >= 0; k -= 2) {
				const bytHex = hexs[j].substr(k, 2)
				if (first) {
					first = false
				} else {
					incStr += ", "
				}
				incStr += "0x" + bytHex.toUpperCase()
			}
		}
	}
	if (name.length == 0) {
		return
	}
	out()
}
