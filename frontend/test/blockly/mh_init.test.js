import { describe, it, expect, vi } from 'vitest'
import { definition } from '../../src/components/blockly/mh_init.js'

function makeBlock(fields = {}, values = {}, statements = {}, extra = {}) {
    return {
        getFieldValue: (name) => fields[name] ?? null,
        id: extra.id ?? 'test-block-id',
        outputConnection: extra.outputConnection ?? null,
        ...extra
    }
}

function makeGenerator(values = {}, statements = {}) {
    return {
        valueToCode: (block, name, _order) => values[name] ?? '',
        statementToCode: (block, name) => statements[name] ?? ''
    }
}

describe('mh_init block definition', () => {
    it('has category "Control flow"', () => {
        expect(definition.category).toBe('Control flow')
    })

    it('has type "mh_init"', () => {
        expect(definition.blockdefinition.type).toBe('mh_init')
    })

    it('does NOT have previousStatement or nextStatement', () => {
        expect(definition.blockdefinition.previousStatement).toBeUndefined()
        expect(definition.blockdefinition.nextStatement).toBeUndefined()
    })

    it('has no output field', () => {
        expect(definition.blockdefinition.output).toBeUndefined()
    })
})

describe('mh_init generator', () => {
    it('generates hub.init wrapper with empty body', () => {
        const block = makeBlock()
        const gen = makeGenerator()
        const code = definition.generator(block, gen)
        expect(code).toBe('hub.init(function()\nend)\n')
    })

    it('generates hub.init wrapper with DO body', () => {
        const block = makeBlock()
        const gen = makeGenerator({}, { DO: '  doSomething()\n' })
        const code = definition.generator(block, gen)
        expect(code).toBe('hub.init(function()\n  doSomething()\nend)\n')
    })

    it('wraps body in function() ... end)', () => {
        const block = makeBlock()
        const gen = makeGenerator({}, { DO: '  x = 1\n' })
        const code = definition.generator(block, gen)
        expect(code).toContain('hub.init(function()')
        expect(code).toContain('end)\n')
        expect(code).toContain('x = 1')
    })
})
