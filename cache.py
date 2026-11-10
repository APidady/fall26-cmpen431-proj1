from collections import deque, OrderedDict
from utils import Level
import math


class CacheLevel(Level):
    def __init__(self, size, block_size, associativity, eviction_policy, write_policy, level_name, higher_level=None, lower_level=None):
        super().__init__(level_name, higher_level, lower_level)
        self.size = size
        self.block_size = block_size
        self.associativity = associativity
        self.eviction_policy = eviction_policy  # FIFO | LRU | MRU
        self.write_policy = write_policy  # always WB for this assignment

        # define other structures / metadata here (hint, check the imports)
        # todo define number of sets
        # todo define metadata structures (tags, dirty bits, etc...)
        # todo anything else you may want...
        self.block_offset_bits = int(math.log2(block_size))
        self.num_sets = size // (block_size * associativity)
        self.index_bits = int(math.log2(self.num_sets)) if self.num_sets > 1 else 0
        self.tag_bits = 32 - self.index_bits - self.block_offset_bits

        self.sets = [OrderedDict() for _ in range(self.num_sets)]

    def _calculate_index(self, address):
        #return 0  # todo
        if self.index_bits == 0:
            return 0
        return (address >> self.block_offset_bits) & ((1 << self.index_bits) - 1)

    def _calculate_tag(self, address):
        #return 0  # todo
        return address >> (self.block_offset_bits + self.index_bits)

    def _calculate_block_address(self, address):
        #return 0  # todo
        return address & ~(self.block_size - 1)

    def _calculate_block_address_from_tag_index(self, tag, cache_index):
        #return 0  # todo
        return ((tag << self.index_bits) | cache_index) << self.block_offset_bits

    def is_dirty(self, block_address):
        #return True|False  # todo
        index = self._calculate_index(block_address)
        tag = self._calculate_tag(block_address)
        if tag in self.sets[index]:
            return self.sets[index][tag]['dirty']
        if self.higher_level and hasattr(self.higher_level, 'is_dirty'):
            return self.higher_level.is_dirty(block_address)
        return False

    def access(self, operation, address):
        """
        Perform a memory access to the given address. Operations are R for reads, W for writes, and B for block
        / writeback updates. B-type operations do not modify the eviction policy meta-details. To perform an access,
        check to see if the address is in this level. If it is a hit, then report the hit and update the eviction policy
        (if required). If the operation was a store make sure to set the block's dirty bits. If the access was a miss,
        report the miss and check if this set needs to evict a block. If it does, evict the target block. Now that the
        target is evicted allocate space for the next block, update the dirty bits (if required) and perform a read
        on the higher levels to propagate the block. Finally, if the block fetched from the higher block is dirty, set
        our block to also be dirty.
        """
        #pass  # todo
        index = self._calculate_index(address)
        tag = self._calculate_tag(address)
        block_address = self._calculate_block_address(address)

        if tag in self.sets[index]:
            # Hit
            self.report_hit(operation, address)
            if operation != 'B':
                if self.eviction_policy in {'LRU', 'MRU'}:
                    self.sets[index].move_to_end(tag)
            if operation == 'W':
                self.sets[index][tag]['dirty'] = True
        else:
            # Miss
            self.report_miss(operation, address)
            if len(self.sets[index]) >= self.associativity:
                self.evict(index)
            
            # Allocate space for the new block
            self.sets[index][tag] = {'dirty': False}
            if operation != 'B':
                if self.eviction_policy in {'LRU', 'MRU'}:
                    self.sets[index].move_to_end(tag)
            
            # Transfer to higher memory level
            if self.higher_level:
                self.higher_level.access('R' if operation != 'B' else 'B', address)
            
            # If block fetched from the higher block is dirty, mark block as dirty
            if self.higher_level and hasattr(self.higher_level, 'is_dirty'):
                if self.higher_level.is_dirty(block_address):
                    self.sets[index][tag]['dirty'] = True

            if operation == 'W':
                self.sets[index][tag]['dirty'] = True

    def evict(self, cache_index):
        """
        Select a victim block in the given way provided this level's eviction policy. Calculate its block address and
        then invalidate said block.
        """
        # select a victim block in the set provided the eviction policy (FIFO | LRU | MRU)
        #victim_block_tag = None  # todo
        if self.eviction_policy in {'FIFO', 'LRU'}:
            victim_block_tag = next(iter(self.sets[cache_index]))
        elif self.eviction_policy == 'MRU':
            victim_block_tag = list(self.sets[cache_index].keys())[-1]
        else:
            victim_block_tag = next(iter(self.sets[cache_index]))

        # invalidate the block
        evicted_block = self._calculate_block_address_from_tag_index(victim_block_tag, cache_index)
        self.invalidate(evicted_block)

    def invalidate(self, block_address):
        """
        Invalidate the block given by block address. If the block is not in this level, then we know it is not in
        lower levels. If it is in this level, then we need to invalidate lower levels first since they may be dirty.
        Once all lower levels have been invalidated, we need to check if our level is dirty, and if it is, perform a
        writeback and report that. Finally, once all lower levels have been invalidated we can remove the block from
        our level and report the eviction.
        """
        #pass  # todo
        index = self._calculate_index(block_address)
        tag = self._calculate_tag(block_address)

        if tag not in self.sets[index]:
            return

        # Invalidate lower levels (CPU-side / higher hierarchy level) first due to inclusion property
        if self.lower_level and hasattr(self.lower_level, 'invalidate'):
            self.lower_level.invalidate(block_address)

        # Check if our level is dirty and perform writeback if necessary
        if self.sets[index][tag]['dirty']:
            self.report_writeback(block_address)
            if self.higher_level:
                self.higher_level.access('B', block_address)

        # Remove the block from our level and report eviction
        del self.sets[index][tag]
        self.report_eviction(block_address)
