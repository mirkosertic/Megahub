import { describe, it, expect, vi } from 'vitest'
import { definition } from '../../src/components/blockly/mh_alg_pid.js'

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

describe('mh_alg_pid block definition', () => {
    it('has category "Algorithms"', () => {
        expect(definition.category).toBe('Algorithms')
    })

    it('has type "mh_alg_pid"', () => {
        expect(definition.blockdefinition.type).toBe('mh_alg_pid')
    })

    it('has output (expression block)', () => {
        expect('output' in definition.blockdefinition).toBe(true)
    })
})

describe('mh_alg_pid generator', () => {
    it('generates correct code with all values provided', () => {
        const block = makeBlock({}, {}, {}, { id: 'test-id' })
        const gen = makeGenerator({
            SETPOINT: '100',
            PV: '0',
            KP: '1.5',
            KI: '0.1',
            KD: '0.05',
            OUT_MIN: '-127',
            OUT_MAX: '128'
        })
        const result = definition.generator(block, gen)
        expect(result[0]).toBe('alg.PID("PID_test-id", 100, 0, 1.5, 0.1, 0.05, -127, 128)')
        expect(result[1]).toBe(0)
    })

    it('uses defaults when valueToCode returns empty string', () => {
        const block = makeBlock({}, {}, {}, { id: 'test-id' })
        const gen = makeGenerator({})
        const result = definition.generator(block, gen)
        expect(result[0]).toBe('alg.PID("PID_test-id", 0, 0, 1.0, 0.0, 0.0, -100, 100)')
        expect(result[1]).toBe(0)
    })

    it('embeds block.id in the PID key', () => {
        const block = makeBlock({}, {}, {}, { id: 'my-unique-id' })
        const gen = makeGenerator({})
        const result = definition.generator(block, gen)
        expect(result[0]).toContain('"PID_my-unique-id"')
    })

    it('returns array [command, 0]', () => {
        const block = makeBlock({}, {}, {}, { id: 'test-id' })
        const gen = makeGenerator({})
        const result = definition.generator(block, gen)
        expect(Array.isArray(result)).toBe(true)
        expect(result.length).toBe(2)
        expect(result[1]).toBe(0)
    })
})
