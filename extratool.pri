defineReplace(superBaseName) {
    THEMEDIR = $$dirname($$1)
    return($$basename(THEMEDIR)/$$basename($$1))
}
